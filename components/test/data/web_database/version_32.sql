PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY,value LONGVARCHAR);
INSERT INTO "meta" VALUES('version','32');
INSERT INTO "meta" VALUES('last_compatible_version','32');
INSERT INTO "meta" VALUES('Default Search Provider ID','2');
INSERT INTO "meta" VALUES('Builtin Keyword Version','33');
CREATE TABLE keywords (id INTEGER PRIMARY KEY,short_name VARCHAR NOT NULL,keyword VARCHAR NOT NULL,favicon_url VARCHAR NOT NULL,url VARCHAR NOT NULL,show_in_default_list INTEGER,safe_for_autoreplace INTEGER,originating_url VARCHAR,date_created INTEGER DEFAULT 0,usage_count INTEGER DEFAULT 0,input_encodings VARCHAR,suggest_url VARCHAR,prepopulate_id INTEGER DEFAULT 0,autogenerate_keyword INTEGER DEFAULT 0,logo_id INTEGER DEFAULT 0,created_by_policy INTEGER DEFAULT 0,instant_url VARCHAR);
INSERT INTO "keywords" VALUES(2,'Google','google.com','http://www.google.com/favicon.ico','{google:baseURL}search?{google:RLZ}{google:acceptedSuggestion}{google:originalQueryForSuggestion}sourceid=chrome&ie={inputEncoding}&q={searchTerms}',1,1,'',0,0,'UTF-8','{google:baseSuggestURL}search?client=chrome&hl={language}&q={searchTerms}',1,1,6256,0,'{google:baseURL}webhp?{google:RLZ}sourceid=chrome-instant&ie={inputEncoding}&ion=1{searchTerms}&nord=1');
INSERT INTO "keywords" VALUES(3,'Yahoo!','yahoo.com','http://search.yahoo.com/favicon.ico','http://search.yahoo.com/search?ei={inputEncoding}&fr=crmas&p={searchTerms}',1,1,'',0,0,'UTF-8','http://ff.search.yahoo.com/gossip?output=fxjson&command={searchTerms}',2,0,6273,0,'');
INSERT INTO "keywords" VALUES(4,'Bing','bing.com','http://www.bing.com/s/wlflag.ico','http://www.bing.com/search?setmkt=en-US&q={searchTerms}',1,1,'',0,0,'UTF-8','http://api.bing.com/osjson.aspx?query={searchTerms}&language={language}',3,0,6250,0,'');
CREATE TABLE logins (origin_url VARCHAR NOT NULL, action_url VARCHAR, username_element VARCHAR, username_value VARCHAR, password_element VARCHAR, password_value BLOB, submit_element VARCHAR, signon_realm VARCHAR NOT NULL,ssl_valid INTEGER NOT NULL,preferred INTEGER NOT NULL,date_created INTEGER NOT NULL,blacklisted_by_user INTEGER NOT NULL,scheme INTEGER NOT NULL,UNIQUE (origin_url, username_element, username_value, password_element, submit_element, signon_realm));
CREATE TABLE web_app_icons (url LONGVARCHAR,width int,height int,image BLOB, UNIQUE (url, width, height));
CREATE TABLE web_apps (url LONGVARCHAR UNIQUE,has_all_images INTEGER NOT NULL);
CREATE TABLE autofill (name VARCHAR, value VARCHAR, value_lower VARCHAR, pair_id INTEGER PRIMARY KEY, count INTEGER DEFAULT 1);
CREATE TABLE autofill_dates ( pair_id INTEGER DEFAULT 0, date_created INTEGER DEFAULT 0);
CREATE TABLE autofill_profiles ( guid VARCHAR PRIMARY KEY, label VARCHAR, first_name VARCHAR, middle_name VARCHAR, last_name VARCHAR, email VARCHAR, company_name VARCHAR, address_line_1 VARCHAR, address_line_2 VARCHAR, city VARCHAR, state VARCHAR, zipcode VARCHAR, country VARCHAR, phone VARCHAR, fax VARCHAR, date_modified INTEGER NOT NULL DEFAULT 0);
INSERT INTO "autofill_profiles" VALUES('00580526-FF81-EE2A-0546-1AC593A32E2F','John Doe, 1 Main St','John','','Doe','john@doe.com','Doe Enterprises','1 Main St','Apt 1','Los Altos','CA','94022','USA','4151112222','4153334444',1297882100);
INSERT INTO "autofill_profiles" VALUES('589636FD-9037-3053-200C-80ABC97D7344','John P. Doe, 1 Main St','John','P.','Doe','john@doe.com','Doe Enterprises','1 Main St','Apt 1','Los Altos','CA','94022','USA','4151112222','4153334444',1297882100);
INSERT INTO "autofill_profiles" VALUES('4C74A9D8-7EEE-423E-F9C2-E7FA70ED1396','Dave Smith, 2 Main Street','Dave','','Smith','','','2 Main Street','','Los Altos','CA','94022','USA','','',1297882100);
INSERT INTO "autofill_profiles" VALUES('722DF5C4-F74A-294A-46F0-31FFDED0D635','Dave Smith, 2 Main St','Dave','','Smith','','','2 Main St','','Los Altos','CA','94022','USA','','',1297882100);
INSERT INTO "autofill_profiles" VALUES('584282AC-5D21-8D73-A2DB-4F892EF61F3F','Alfred E Newman, a@e.com','Alfred','E','Newman','a@e.com','','','','','','','','','',1297882100);
INSERT INTO "autofill_profiles" VALUES('9E5FE298-62C7-83DF-6293-381BC589183F','3 Main St, Los Altos','','','','','','3 Main St','','Los Altos','CA','94022','USA','','',1297882100);
CREATE TABLE credit_cards ( guid VARCHAR PRIMARY KEY, label VARCHAR, name_on_card VARCHAR, expiration_month INTEGER, expiration_year INTEGER, card_number_encrypted BLOB, date_modified INTEGER NOT NULL DEFAULT 0);
CREATE TABLE token_service (service VARCHAR PRIMARY KEY NOT NULL,encrypted_token BLOB);
CREATE INDEX logins_signon ON logins (signon_realm);
CREATE INDEX web_apps_url_index ON web_apps (url);
CREATE INDEX autofill_name ON autofill (name);
CREATE INDEX autofill_name_value_lower ON autofill (name, value_lower);
CREATE INDEX autofill_dates_pair_id ON autofill_dates (pair_id);
CREATE INDEX autofill_profiles_label_index ON autofill_profiles (label);
CREATE INDEX credit_cards_label_index ON credit_cards (label);
COMMIT;
