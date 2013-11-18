// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/configuration_policy_provider.h"

#include "base/callback.h"
#include "chrome/browser/policy/external_data_fetcher.h"
#include "chrome/browser/policy/policy_map.h"
#include "policy/policy_constants.h"

namespace policy {

namespace {

const char* kProxyPolicies[] = {
  key::kProxyMode,
  key::kProxyServerMode,
  key::kProxyServer,
  key::kProxyPacUrl,
  key::kProxyBypassList,
};

// Helper that converts deprecated chrome policies into their corresponding
// actual policies.
void FixDeprecatedPolicies(PolicyMap* policies) {
  // Proxy settings have been configured by 5 policies that didn't mix well
  // together, and maps of policies had to take this into account when merging
  // policy sources. The proxy settings will eventually be configured by a
  // single Dictionary policy when all providers have support for that. For
  // now, the individual policies are mapped here to a single Dictionary policy
  // that the rest of the policy machinery uses.

  // The highest (level, scope) pair for an existing proxy policy is determined
  // first, and then only policies with those exact attributes are merged.
  PolicyMap::Entry current_priority;  // Defaults to the lowest priority.
  scoped_ptr<DictionaryValue> proxy_settings(new DictionaryValue);
  for (size_t i = 0; i < arraysize(kProxyPolicies); ++i) {
    const PolicyMap::Entry* entry = policies->Get(kProxyPolicies[i]);
    if (entry) {
      if (entry->has_higher_priority_than(current_priority)) {
        proxy_settings->Clear();
        current_priority = *entry;
      }
      if (!entry->has_higher_priority_than(current_priority) &&
          !current_priority.has_higher_priority_than(*entry)) {
        proxy_settings->Set(kProxyPolicies[i], entry->value->DeepCopy());
      }
      policies->Erase(kProxyPolicies[i]);
    }
  }
  // Sets the new |proxy_settings| if kProxySettings isn't set yet, or if the
  // new priority is higher.
  const PolicyMap::Entry* existing = policies->Get(key::kProxySettings);
  if (!proxy_settings->empty() &&
      (!existing || current_priority.has_higher_priority_than(*existing))) {
    policies->Set(key::kProxySettings,
                  current_priority.level,
                  current_priority.scope,
                  proxy_settings.release(),
                  NULL);
  }
}

}  // namespace

ConfigurationPolicyProvider::Observer::~Observer() {}

ConfigurationPolicyProvider::ConfigurationPolicyProvider()
    : did_shutdown_(false),
      schema_registry_(NULL) {}

ConfigurationPolicyProvider::~ConfigurationPolicyProvider() {
  DCHECK(did_shutdown_);
}

void ConfigurationPolicyProvider::Init(SchemaRegistry* registry) {
  schema_registry_ = registry;
  schema_registry_->AddObserver(this);
}

void ConfigurationPolicyProvider::Shutdown() {
  did_shutdown_ = true;
  if (schema_registry_) {
    // Unit tests don't initialize the BrowserPolicyConnector but call
    // shutdown; handle that.
    schema_registry_->RemoveObserver(this);
    schema_registry_ = NULL;
  }
}

bool ConfigurationPolicyProvider::IsInitializationComplete(
    PolicyDomain domain) const {
  return true;
}

void ConfigurationPolicyProvider::UpdatePolicy(
    scoped_ptr<PolicyBundle> bundle) {
  if (bundle.get())
    policy_bundle_.Swap(bundle.get());
  else
    policy_bundle_.Clear();
  FixDeprecatedPolicies(&policy_bundle_.Get(
      PolicyNamespace(POLICY_DOMAIN_CHROME, std::string())));
  FOR_EACH_OBSERVER(ConfigurationPolicyProvider::Observer,
                    observer_list_,
                    OnUpdatePolicy(this));
}

SchemaRegistry* ConfigurationPolicyProvider::schema_registry() const {
  return schema_registry_;
}

const scoped_refptr<SchemaMap>&
ConfigurationPolicyProvider::schema_map() const {
  return schema_registry_->schema_map();
}

void ConfigurationPolicyProvider::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ConfigurationPolicyProvider::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void ConfigurationPolicyProvider::OnSchemaRegistryUpdated(
    bool has_new_schemas) {}

void ConfigurationPolicyProvider::OnSchemaRegistryReady() {}

}  // namespace policy
