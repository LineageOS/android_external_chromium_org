// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/test/error_callback_support.h"
#include "sql/test/scoped_error_ignorer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/sqlite/sqlite3.h"

namespace {

// Helper to return the count of items in sqlite_master.  Return -1 in
// case of error.
int SqliteMasterCount(sql::Connection* db) {
  const char* kMasterCount = "SELECT COUNT(*) FROM sqlite_master";
  sql::Statement s(db->GetUniqueStatement(kMasterCount));
  return s.Step() ? s.ColumnInt(0) : -1;
}

// Track the number of valid references which share the same pointer.
// This is used to allow testing an implicitly use-after-free case by
// explicitly having the ref count live longer than the object.
class RefCounter {
 public:
  RefCounter(size_t* counter)
      : counter_(counter) {
    (*counter_)++;
  }
  RefCounter(const RefCounter& other)
      : counter_(other.counter_) {
    (*counter_)++;
  }
  ~RefCounter() {
    (*counter_)--;
  }

 private:
  size_t* counter_;

  DISALLOW_ASSIGN(RefCounter);
};

// Empty callback for implementation of ErrorCallbackSetHelper().
void IgnoreErrorCallback(int error, sql::Statement* stmt) {
}

void ErrorCallbackSetHelper(sql::Connection* db,
                            size_t* counter,
                            const RefCounter& r,
                            int error, sql::Statement* stmt) {
  // The ref count should not go to zero when changing the callback.
  EXPECT_GT(*counter, 0u);
  db->set_error_callback(base::Bind(&IgnoreErrorCallback));
  EXPECT_GT(*counter, 0u);
}

void ErrorCallbackResetHelper(sql::Connection* db,
                              size_t* counter,
                              const RefCounter& r,
                              int error, sql::Statement* stmt) {
  // The ref count should not go to zero when clearing the callback.
  EXPECT_GT(*counter, 0u);
  db->reset_error_callback();
  EXPECT_GT(*counter, 0u);
}

#if defined(OS_POSIX)
// Set a umask and restore the old mask on destruction.  Cribbed from
// shared_memory_unittest.cc.  Used by POSIX-only UserPermission test.
class ScopedUmaskSetter {
 public:
  explicit ScopedUmaskSetter(mode_t target_mask) {
    old_umask_ = umask(target_mask);
  }
  ~ScopedUmaskSetter() { umask(old_umask_); }
 private:
  mode_t old_umask_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedUmaskSetter);
};
#endif

class SQLConnectionTest : public testing::Test {
 public:
  virtual void SetUp() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    db_path_ = temp_dir_.path().AppendASCII("SQLConnectionTest.db");
    ASSERT_TRUE(db_.Open(db_path_));
  }

  virtual void TearDown() {
    db_.Close();
  }

  sql::Connection& db() { return db_; }
  const base::FilePath& db_path() { return db_path_; }

  // Handle errors by blowing away the database.
  void RazeErrorCallback(int expected_error, int error, sql::Statement* stmt) {
    EXPECT_EQ(expected_error, error);
    db_.RazeAndClose();
  }

 private:
  sql::Connection db_;
  base::FilePath db_path_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(SQLConnectionTest, Execute) {
  // Valid statement should return true.
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  EXPECT_EQ(SQLITE_OK, db().GetErrorCode());

  // Invalid statement should fail.
  ASSERT_EQ(SQLITE_ERROR,
            db().ExecuteAndReturnErrorCode("CREATE TAB foo (a, b"));
  EXPECT_EQ(SQLITE_ERROR, db().GetErrorCode());
}

TEST_F(SQLConnectionTest, ExecuteWithErrorCode) {
  ASSERT_EQ(SQLITE_OK,
            db().ExecuteAndReturnErrorCode("CREATE TABLE foo (a, b)"));
  ASSERT_EQ(SQLITE_ERROR,
            db().ExecuteAndReturnErrorCode("CREATE TABLE TABLE"));
  ASSERT_EQ(SQLITE_ERROR,
            db().ExecuteAndReturnErrorCode(
                "INSERT INTO foo(a, b) VALUES (1, 2, 3, 4)"));
}

TEST_F(SQLConnectionTest, CachedStatement) {
  sql::StatementID id1("foo", 12);

  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  ASSERT_TRUE(db().Execute("INSERT INTO foo(a, b) VALUES (12, 13)"));

  // Create a new cached statement.
  {
    sql::Statement s(db().GetCachedStatement(id1, "SELECT a FROM foo"));
    ASSERT_TRUE(s.is_valid());

    ASSERT_TRUE(s.Step());
    EXPECT_EQ(12, s.ColumnInt(0));
  }

  // The statement should be cached still.
  EXPECT_TRUE(db().HasCachedStatement(id1));

  {
    // Get the same statement using different SQL. This should ignore our
    // SQL and use the cached one (so it will be valid).
    sql::Statement s(db().GetCachedStatement(id1, "something invalid("));
    ASSERT_TRUE(s.is_valid());

    ASSERT_TRUE(s.Step());
    EXPECT_EQ(12, s.ColumnInt(0));
  }

  // Make sure other statements aren't marked as cached.
  EXPECT_FALSE(db().HasCachedStatement(SQL_FROM_HERE));
}

TEST_F(SQLConnectionTest, IsSQLValidTest) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  ASSERT_TRUE(db().IsSQLValid("SELECT a FROM foo"));
  ASSERT_FALSE(db().IsSQLValid("SELECT no_exist FROM foo"));
}

TEST_F(SQLConnectionTest, DoesStuffExist) {
  // Test DoesTableExist.
  EXPECT_FALSE(db().DoesTableExist("foo"));
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));
  EXPECT_TRUE(db().DoesTableExist("foo"));

  // Should be case sensitive.
  EXPECT_FALSE(db().DoesTableExist("FOO"));

  // Test DoesColumnExist.
  EXPECT_FALSE(db().DoesColumnExist("foo", "bar"));
  EXPECT_TRUE(db().DoesColumnExist("foo", "a"));

  // Testing for a column on a nonexistent table.
  EXPECT_FALSE(db().DoesColumnExist("bar", "b"));
}

TEST_F(SQLConnectionTest, GetLastInsertRowId) {
  ASSERT_TRUE(db().Execute("CREATE TABLE foo (id INTEGER PRIMARY KEY, value)"));

  ASSERT_TRUE(db().Execute("INSERT INTO foo (value) VALUES (12)"));

  // Last insert row ID should be valid.
  int64 row = db().GetLastInsertRowId();
  EXPECT_LT(0, row);

  // It should be the primary key of the row we just inserted.
  sql::Statement s(db().GetUniqueStatement("SELECT value FROM foo WHERE id=?"));
  s.BindInt64(0, row);
  ASSERT_TRUE(s.Step());
  EXPECT_EQ(12, s.ColumnInt(0));
}

TEST_F(SQLConnectionTest, Rollback) {
  ASSERT_TRUE(db().BeginTransaction());
  ASSERT_TRUE(db().BeginTransaction());
  EXPECT_EQ(2, db().transaction_nesting());
  db().RollbackTransaction();
  EXPECT_FALSE(db().CommitTransaction());
  EXPECT_TRUE(db().BeginTransaction());
}

// Test the scoped error ignorer by attempting to insert a duplicate
// value into an index.
TEST_F(SQLConnectionTest, ScopedIgnoreError) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER UNIQUE)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (id) VALUES (12)"));

  sql::ScopedErrorIgnorer ignore_errors;
  ignore_errors.IgnoreError(SQLITE_CONSTRAINT);
  ASSERT_FALSE(db().Execute("INSERT INTO foo (id) VALUES (12)"));
  ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
}

TEST_F(SQLConnectionTest, ErrorCallback) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER UNIQUE)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (id) VALUES (12)"));

  int error = SQLITE_OK;
  {
    sql::ScopedErrorCallback sec(
        &db(), base::Bind(&sql::CaptureErrorCallback, &error));
    EXPECT_FALSE(db().Execute("INSERT INTO foo (id) VALUES (12)"));
    EXPECT_EQ(SQLITE_CONSTRAINT, error);
  }

  // Callback is no longer in force due to reset.
  {
    error = SQLITE_OK;
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_CONSTRAINT);
    ASSERT_FALSE(db().Execute("INSERT INTO foo (id) VALUES (12)"));
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
    EXPECT_EQ(SQLITE_OK, error);
  }

  // base::Bind() can curry arguments to be passed by const reference
  // to the callback function.  If the callback function calls
  // re/set_error_callback(), the storage for those arguments can be
  // deleted while the callback function is still executing.
  //
  // RefCounter() counts how many objects are live using an external
  // count.  The same counter is passed to the callback, so that it
  // can check directly even if the RefCounter object is no longer
  // live.
  {
    size_t count = 0;
    sql::ScopedErrorCallback sec(
        &db(), base::Bind(&ErrorCallbackSetHelper,
                          &db(), &count, RefCounter(&count)));

    EXPECT_FALSE(db().Execute("INSERT INTO foo (id) VALUES (12)"));
  }

  // Same test, but reset_error_callback() case.
  {
    size_t count = 0;
    sql::ScopedErrorCallback sec(
        &db(), base::Bind(&ErrorCallbackResetHelper,
                          &db(), &count, RefCounter(&count)));

    EXPECT_FALSE(db().Execute("INSERT INTO foo (id) VALUES (12)"));
  }
}

// Test that sql::Connection::Raze() results in a database without the
// tables from the original database.
TEST_F(SQLConnectionTest, Raze) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute("INSERT INTO foo (value) VALUES (12)"));

  int pragma_auto_vacuum = 0;
  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA auto_vacuum"));
    ASSERT_TRUE(s.Step());
    pragma_auto_vacuum = s.ColumnInt(0);
    ASSERT_TRUE(pragma_auto_vacuum == 0 || pragma_auto_vacuum == 1);
  }

  // If auto_vacuum is set, there's an extra page to maintain a freelist.
  const int kExpectedPageCount = 2 + pragma_auto_vacuum;

  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA page_count"));
    ASSERT_TRUE(s.Step());
    EXPECT_EQ(kExpectedPageCount, s.ColumnInt(0));
  }

  {
    sql::Statement s(db().GetUniqueStatement("SELECT * FROM sqlite_master"));
    ASSERT_TRUE(s.Step());
    EXPECT_EQ("table", s.ColumnString(0));
    EXPECT_EQ("foo", s.ColumnString(1));
    EXPECT_EQ("foo", s.ColumnString(2));
    // Table "foo" is stored in the last page of the file.
    EXPECT_EQ(kExpectedPageCount, s.ColumnInt(3));
    EXPECT_EQ(kCreateSql, s.ColumnString(4));
  }

  ASSERT_TRUE(db().Raze());

  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA page_count"));
    ASSERT_TRUE(s.Step());
    EXPECT_EQ(1, s.ColumnInt(0));
  }

  ASSERT_EQ(0, SqliteMasterCount(&db()));

  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA auto_vacuum"));
    ASSERT_TRUE(s.Step());
    // The new database has the same auto_vacuum as a fresh database.
    EXPECT_EQ(pragma_auto_vacuum, s.ColumnInt(0));
  }
}

// Test that Raze() maintains page_size.
TEST_F(SQLConnectionTest, RazePageSize) {
  // Fetch the default page size and double it for use in this test.
  // Scoped to release statement before Close().
  int default_page_size = 0;
  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA page_size"));
    ASSERT_TRUE(s.Step());
    default_page_size = s.ColumnInt(0);
  }
  ASSERT_GT(default_page_size, 0);
  const int kPageSize = 2 * default_page_size;

  // Re-open the database to allow setting the page size.
  db().Close();
  db().set_page_size(kPageSize);
  ASSERT_TRUE(db().Open(db_path()));

  // page_size should match the indicated value.
  sql::Statement s(db().GetUniqueStatement("PRAGMA page_size"));
  ASSERT_TRUE(s.Step());
  ASSERT_EQ(kPageSize, s.ColumnInt(0));

  // After raze, page_size should still match the indicated value.
  ASSERT_TRUE(db().Raze());
  s.Reset(true);
  ASSERT_TRUE(s.Step());
  ASSERT_EQ(kPageSize, s.ColumnInt(0));
}

// Test that Raze() results are seen in other connections.
TEST_F(SQLConnectionTest, RazeMultiple) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));

  sql::Connection other_db;
  ASSERT_TRUE(other_db.Open(db_path()));

  // Check that the second connection sees the table.
  ASSERT_EQ(1, SqliteMasterCount(&other_db));

  ASSERT_TRUE(db().Raze());

  // The second connection sees the updated database.
  ASSERT_EQ(0, SqliteMasterCount(&other_db));
}

TEST_F(SQLConnectionTest, RazeLocked) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));

  // Open a transaction and write some data in a second connection.
  // This will acquire a PENDING or EXCLUSIVE transaction, which will
  // cause the raze to fail.
  sql::Connection other_db;
  ASSERT_TRUE(other_db.Open(db_path()));
  ASSERT_TRUE(other_db.BeginTransaction());
  const char* kInsertSql = "INSERT INTO foo VALUES (1, 'data')";
  ASSERT_TRUE(other_db.Execute(kInsertSql));

  ASSERT_FALSE(db().Raze());

  // Works after COMMIT.
  ASSERT_TRUE(other_db.CommitTransaction());
  ASSERT_TRUE(db().Raze());

  // Re-create the database.
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute(kInsertSql));

  // An unfinished read transaction in the other connection also
  // blocks raze.
  const char *kQuery = "SELECT COUNT(*) FROM foo";
  sql::Statement s(other_db.GetUniqueStatement(kQuery));
  ASSERT_TRUE(s.Step());
  ASSERT_FALSE(db().Raze());

  // Complete the statement unlocks the database.
  ASSERT_FALSE(s.Step());
  ASSERT_TRUE(db().Raze());
}

// Verify that Raze() can handle an empty file.  SQLite should treat
// this as an empty database.
TEST_F(SQLConnectionTest, RazeEmptyDB) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  db().Close();

  {
    file_util::ScopedFILE file(file_util::OpenFile(db_path(), "rb+"));
    ASSERT_TRUE(file.get() != NULL);
    ASSERT_EQ(0, fseek(file.get(), 0, SEEK_SET));
    ASSERT_TRUE(file_util::TruncateFile(file.get()));
  }

  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_TRUE(db().Raze());
  EXPECT_EQ(0, SqliteMasterCount(&db()));
}

// Verify that Raze() can handle a file of junk.
TEST_F(SQLConnectionTest, RazeNOTADB) {
  db().Close();
  sql::Connection::Delete(db_path());
  ASSERT_FALSE(base::PathExists(db_path()));

  {
    file_util::ScopedFILE file(file_util::OpenFile(db_path(), "wb"));
    ASSERT_TRUE(file.get() != NULL);

    const char* kJunk = "This is the hour of our discontent.";
    fputs(kJunk, file.get());
  }
  ASSERT_TRUE(base::PathExists(db_path()));

  // SQLite will successfully open the handle, but will fail with
  // SQLITE_IOERR_SHORT_READ on pragma statemenets which read the
  // header.
  {
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_IOERR_SHORT_READ);
    EXPECT_TRUE(db().Open(db_path()));
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
  }
  EXPECT_TRUE(db().Raze());
  db().Close();

  // Now empty, the open should open an empty database.
  EXPECT_TRUE(db().Open(db_path()));
  EXPECT_EQ(0, SqliteMasterCount(&db()));
}

// Verify that Raze() can handle a database overwritten with garbage.
TEST_F(SQLConnectionTest, RazeNOTADB2) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_EQ(1, SqliteMasterCount(&db()));
  db().Close();

  {
    file_util::ScopedFILE file(file_util::OpenFile(db_path(), "rb+"));
    ASSERT_TRUE(file.get() != NULL);
    ASSERT_EQ(0, fseek(file.get(), 0, SEEK_SET));

    const char* kJunk = "This is the hour of our discontent.";
    fputs(kJunk, file.get());
  }

  // SQLite will successfully open the handle, but will fail with
  // SQLITE_NOTADB on pragma statemenets which attempt to read the
  // corrupted header.
  {
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_NOTADB);
    EXPECT_TRUE(db().Open(db_path()));
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
  }
  EXPECT_TRUE(db().Raze());
  db().Close();

  // Now empty, the open should succeed with an empty database.
  EXPECT_TRUE(db().Open(db_path()));
  EXPECT_EQ(0, SqliteMasterCount(&db()));
}

// Test that a callback from Open() can raze the database.  This is
// essential for cases where the Open() can fail entirely, so the
// Raze() cannot happen later.  Additionally test that when the
// callback does this during Open(), the open is retried and succeeds.
//
// Most corruptions seen in the wild seem to happen when two pages in
// the database were not written transactionally (the transaction
// changed both, but one wasn't successfully written for some reason).
// A special case of that is when the header indicates that the
// database contains more pages than are in the file.  This breaks
// things at a very basic level, verify that Raze() can handle it.
TEST_F(SQLConnectionTest, RazeCallbackReopen) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_EQ(1, SqliteMasterCount(&db()));
  int page_size = 0;
  {
    sql::Statement s(db().GetUniqueStatement("PRAGMA page_size"));
    ASSERT_TRUE(s.Step());
    page_size = s.ColumnInt(0);
  }
  db().Close();

  // Trim a single page from the end of the file.
  {
    file_util::ScopedFILE file(file_util::OpenFile(db_path(), "rb+"));
    ASSERT_TRUE(file.get() != NULL);
    ASSERT_EQ(0, fseek(file.get(), -page_size, SEEK_END));
    ASSERT_TRUE(file_util::TruncateFile(file.get()));
  }

  // Open() will succeed, even though the PRAGMA calls within will
  // fail with SQLITE_CORRUPT, as will this PRAGMA.
  {
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_CORRUPT);
    ASSERT_TRUE(db().Open(db_path()));
    ASSERT_FALSE(db().Execute("PRAGMA auto_vacuum"));
    db().Close();
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
  }

  db().set_error_callback(base::Bind(&SQLConnectionTest::RazeErrorCallback,
                                     base::Unretained(this),
                                     SQLITE_CORRUPT));

  // When the PRAGMA calls in Open() raise SQLITE_CORRUPT, the error
  // callback will call RazeAndClose().  Open() will then fail and be
  // retried.  The second Open() on the empty database will succeed
  // cleanly.
  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_TRUE(db().Execute("PRAGMA auto_vacuum"));
  EXPECT_EQ(0, SqliteMasterCount(&db()));
}

// Basic test of RazeAndClose() operation.
TEST_F(SQLConnectionTest, RazeAndClose) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  const char* kPopulateSql = "INSERT INTO foo (value) VALUES (12)";

  // Test that RazeAndClose() closes the database, and that the
  // database is empty when re-opened.
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute(kPopulateSql));
  ASSERT_TRUE(db().RazeAndClose());
  ASSERT_FALSE(db().is_open());
  db().Close();
  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_EQ(0, SqliteMasterCount(&db()));

  // Test that RazeAndClose() can break transactions.
  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute(kPopulateSql));
  ASSERT_TRUE(db().BeginTransaction());
  ASSERT_TRUE(db().RazeAndClose());
  ASSERT_FALSE(db().is_open());
  ASSERT_FALSE(db().CommitTransaction());
  db().Close();
  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_EQ(0, SqliteMasterCount(&db()));
}

// Test that various operations fail without crashing after
// RazeAndClose().
TEST_F(SQLConnectionTest, RazeAndCloseDiagnostics) {
  const char* kCreateSql = "CREATE TABLE foo (id INTEGER PRIMARY KEY, value)";
  const char* kPopulateSql = "INSERT INTO foo (value) VALUES (12)";
  const char* kSimpleSql = "SELECT 1";

  ASSERT_TRUE(db().Execute(kCreateSql));
  ASSERT_TRUE(db().Execute(kPopulateSql));

  // Test baseline expectations.
  db().Preload();
  ASSERT_TRUE(db().DoesTableExist("foo"));
  ASSERT_TRUE(db().IsSQLValid(kSimpleSql));
  ASSERT_EQ(SQLITE_OK, db().ExecuteAndReturnErrorCode(kSimpleSql));
  ASSERT_TRUE(db().Execute(kSimpleSql));
  ASSERT_TRUE(db().is_open());
  {
    sql::Statement s(db().GetUniqueStatement(kSimpleSql));
    ASSERT_TRUE(s.Step());
  }
  {
    sql::Statement s(db().GetCachedStatement(SQL_FROM_HERE, kSimpleSql));
    ASSERT_TRUE(s.Step());
  }
  ASSERT_TRUE(db().BeginTransaction());
  ASSERT_TRUE(db().CommitTransaction());
  ASSERT_TRUE(db().BeginTransaction());
  db().RollbackTransaction();

  ASSERT_TRUE(db().RazeAndClose());

  // At this point, they should all fail, but not crash.
  db().Preload();
  ASSERT_FALSE(db().DoesTableExist("foo"));
  ASSERT_FALSE(db().IsSQLValid(kSimpleSql));
  ASSERT_EQ(SQLITE_ERROR, db().ExecuteAndReturnErrorCode(kSimpleSql));
  ASSERT_FALSE(db().Execute(kSimpleSql));
  ASSERT_FALSE(db().is_open());
  {
    sql::Statement s(db().GetUniqueStatement(kSimpleSql));
    ASSERT_FALSE(s.Step());
  }
  {
    sql::Statement s(db().GetCachedStatement(SQL_FROM_HERE, kSimpleSql));
    ASSERT_FALSE(s.Step());
  }
  ASSERT_FALSE(db().BeginTransaction());
  ASSERT_FALSE(db().CommitTransaction());
  ASSERT_FALSE(db().BeginTransaction());
  db().RollbackTransaction();

  // Close normally to reset the poisoned flag.
  db().Close();

  // DEATH tests not supported on Android or iOS.
#if !defined(OS_ANDROID) && !defined(OS_IOS)
  // Once the real Close() has been called, various calls enforce API
  // usage by becoming fatal in debug mode.  Since DEATH tests are
  // expensive, just test one of them.
  if (DLOG_IS_ON(FATAL)) {
    ASSERT_DEATH({
        db().IsSQLValid(kSimpleSql);
      }, "Illegal use of connection without a db");
  }
#endif
}

// TODO(shess): Spin up a background thread to hold other_db, to more
// closely match real life.  That would also allow testing
// RazeWithTimeout().

#if defined(OS_ANDROID)
TEST_F(SQLConnectionTest, SetTempDirForSQL) {

  sql::MetaTable meta_table;
  // Below call needs a temporary directory in sqlite3
  // On Android, it can pass only when the temporary directory is set.
  // Otherwise, sqlite3 doesn't find the correct directory to store
  // temporary files and will report the error 'unable to open
  // database file'.
  ASSERT_TRUE(meta_table.Init(&db(), 4, 4));
}
#endif

TEST_F(SQLConnectionTest, Delete) {
  EXPECT_TRUE(db().Execute("CREATE TABLE x (x)"));
  db().Close();

  // Should have both a main database file and a journal file because
  // of journal_mode PERSIST.
  base::FilePath journal(db_path().value() + FILE_PATH_LITERAL("-journal"));
  ASSERT_TRUE(base::PathExists(db_path()));
  ASSERT_TRUE(base::PathExists(journal));

  sql::Connection::Delete(db_path());
  EXPECT_FALSE(base::PathExists(db_path()));
  EXPECT_FALSE(base::PathExists(journal));
}

#if defined(OS_POSIX)
// Test that set_restrict_to_user() trims database permissions so that
// only the owner (and root) can read.
TEST_F(SQLConnectionTest, UserPermission) {
  // If the bots all had a restrictive umask setting such that
  // databases are always created with only the owner able to read
  // them, then the code could break without breaking the tests.
  // Temporarily provide a more permissive umask.
  db().Close();
  sql::Connection::Delete(db_path());
  ASSERT_FALSE(base::PathExists(db_path()));
  ScopedUmaskSetter permissive_umask(S_IWGRP | S_IWOTH);
  ASSERT_TRUE(db().Open(db_path()));

  // Cause the journal file to be created.  If the default
  // journal_mode is changed back to DELETE, then parts of this test
  // will need to be updated.
  EXPECT_TRUE(db().Execute("CREATE TABLE x (x)"));

  base::FilePath journal(db_path().value() + FILE_PATH_LITERAL("-journal"));
  int mode;

  // Given a permissive umask, the database is created with permissive
  // read access for the database and journal.
  ASSERT_TRUE(base::PathExists(db_path()));
  ASSERT_TRUE(base::PathExists(journal));
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(db_path(), &mode));
  ASSERT_NE((mode & file_util::FILE_PERMISSION_USER_MASK), mode);
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(journal, &mode));
  ASSERT_NE((mode & file_util::FILE_PERMISSION_USER_MASK), mode);

  // Re-open with restricted permissions and verify that the modes
  // changed for both the main database and the journal.
  db().Close();
  db().set_restrict_to_user();
  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_TRUE(base::PathExists(db_path()));
  ASSERT_TRUE(base::PathExists(journal));
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(db_path(), &mode));
  ASSERT_EQ((mode & file_util::FILE_PERMISSION_USER_MASK), mode);
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(journal, &mode));
  ASSERT_EQ((mode & file_util::FILE_PERMISSION_USER_MASK), mode);

  // Delete and re-create the database, the restriction should still apply.
  db().Close();
  sql::Connection::Delete(db_path());
  ASSERT_TRUE(db().Open(db_path()));
  ASSERT_TRUE(base::PathExists(db_path()));
  ASSERT_FALSE(base::PathExists(journal));
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(db_path(), &mode));
  ASSERT_EQ((mode & file_util::FILE_PERMISSION_USER_MASK), mode);

  // Verify that journal creation inherits the restriction.
  EXPECT_TRUE(db().Execute("CREATE TABLE x (x)"));
  ASSERT_TRUE(base::PathExists(journal));
  mode = file_util::FILE_PERMISSION_MASK;
  EXPECT_TRUE(file_util::GetPosixFilePermissions(journal, &mode));
  ASSERT_EQ((mode & file_util::FILE_PERMISSION_USER_MASK), mode);
}
#endif  // defined(OS_POSIX)

// Test that errors start happening once Poison() is called.
TEST_F(SQLConnectionTest, Poison) {
  EXPECT_TRUE(db().Execute("CREATE TABLE x (x)"));

  // Before the Poison() call, things generally work.
  EXPECT_TRUE(db().IsSQLValid("INSERT INTO x VALUES ('x')"));
  EXPECT_TRUE(db().Execute("INSERT INTO x VALUES ('x')"));
  {
    sql::Statement s(db().GetUniqueStatement("SELECT COUNT(*) FROM x"));
    ASSERT_TRUE(s.is_valid());
    ASSERT_TRUE(s.Step());
  }

  // Get a statement which is valid before and will exist across Poison().
  sql::Statement valid_statement(
      db().GetUniqueStatement("SELECT COUNT(*) FROM sqlite_master"));
  ASSERT_TRUE(valid_statement.is_valid());
  ASSERT_TRUE(valid_statement.Step());
  valid_statement.Reset(true);

  db().Poison();

  // After the Poison() call, things fail.
  EXPECT_FALSE(db().IsSQLValid("INSERT INTO x VALUES ('x')"));
  EXPECT_FALSE(db().Execute("INSERT INTO x VALUES ('x')"));
  {
    sql::Statement s(db().GetUniqueStatement("SELECT COUNT(*) FROM x"));
    ASSERT_FALSE(s.is_valid());
    ASSERT_FALSE(s.Step());
  }

  // The existing statement has become invalid.
  ASSERT_FALSE(valid_statement.is_valid());
  ASSERT_FALSE(valid_statement.Step());
}

// Test attaching and detaching databases from the connection.
TEST_F(SQLConnectionTest, Attach) {
  EXPECT_TRUE(db().Execute("CREATE TABLE foo (a, b)"));

  // Create a database to attach to.
  base::FilePath attach_path =
      db_path().DirName().AppendASCII("SQLConnectionAttach.db");
  const char kAttachmentPoint[] = "other";
  {
    sql::Connection other_db;
    ASSERT_TRUE(other_db.Open(attach_path));
    EXPECT_TRUE(other_db.Execute("CREATE TABLE bar (a, b)"));
    EXPECT_TRUE(other_db.Execute("INSERT INTO bar VALUES ('hello', 'world')"));
  }

  // Cannot see the attached database, yet.
  EXPECT_FALSE(db().IsSQLValid("SELECT count(*) from other.bar"));

  // Attach fails in a transaction.
  EXPECT_TRUE(db().BeginTransaction());
  {
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_ERROR);
    EXPECT_FALSE(db().AttachDatabase(attach_path, kAttachmentPoint));
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
  }

  // Attach succeeds when the transaction is closed.
  db().RollbackTransaction();
  EXPECT_TRUE(db().AttachDatabase(attach_path, kAttachmentPoint));
  EXPECT_TRUE(db().IsSQLValid("SELECT count(*) from other.bar"));

  // Queries can touch both databases.
  EXPECT_TRUE(db().Execute("INSERT INTO foo SELECT a, b FROM other.bar"));
  {
    sql::Statement s(db().GetUniqueStatement("SELECT COUNT(*) FROM foo"));
    ASSERT_TRUE(s.Step());
    EXPECT_EQ(1, s.ColumnInt(0));
  }

  // Detach also fails in a transaction.
  EXPECT_TRUE(db().BeginTransaction());
  {
    sql::ScopedErrorIgnorer ignore_errors;
    ignore_errors.IgnoreError(SQLITE_ERROR);
    EXPECT_FALSE(db().DetachDatabase(kAttachmentPoint));
    EXPECT_TRUE(db().IsSQLValid("SELECT count(*) from other.bar"));
    ASSERT_TRUE(ignore_errors.CheckIgnoredErrors());
  }

  // Detach succeeds outside of a transaction.
  db().RollbackTransaction();
  EXPECT_TRUE(db().DetachDatabase(kAttachmentPoint));

  EXPECT_FALSE(db().IsSQLValid("SELECT count(*) from other.bar"));
}

}  // namespace
