/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_util.h"

#include <algorithm>

#include "base/containers/flat_map.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuer_info_aliases.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

constexpr int kMaximumIssuerPublicKeys = 3;

bool PublicKeyExists(const IssuerInfo& issuer, const std::string& public_key) {
  const auto iter = issuer.public_keys.find(public_key);
  if (iter == issuer.public_keys.end()) {
    return false;
  }

  return true;
}

}  // namespace

bool IsIssuerValid(const IssuerInfo& issuer) {
  base::flat_map<double, int> buckets;
  for (const auto& public_key : issuer.public_keys) {
    const double bucket = public_key.second;
    buckets[bucket]++;

    const int count = buckets[bucket];
    if (count > kMaximumIssuerPublicKeys) {
      return false;
    }
  }

  return true;
}

void SetIssuers(const IssuersInfo& issuers) {
  AdsClientHelper::Get()->SetIntegerPref(prefs::kIssuerPing, issuers.ping);

  ConfirmationStateManager::Get()->SetIssuers(issuers.issuers);
  ConfirmationStateManager::Get()->Save();
}

IssuersInfo GetIssuers() {
  IssuersInfo issuers;
  issuers.ping = AdsClientHelper::Get()->GetIntegerPref(prefs::kIssuerPing);
  issuers.issuers = ConfirmationStateManager::Get()->GetIssuers();

  return issuers;
}

bool HasIssuers() {
  if (!IssuerExistsForType(IssuerType::kConfirmations) ||
      !IssuerExistsForType(IssuerType::kPayments)) {
    return false;
  }

  return true;
}

bool HasIssuersChanged(const IssuersInfo& issuers) {
  const IssuersInfo& last_issuers = GetIssuers();
  if (issuers == last_issuers) {
    return false;
  }

  return true;
}

bool IssuerExistsForType(const IssuerType issuer_type) {
  const IssuersInfo& issuers = GetIssuers();

  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, issuer_type);
  if (!issuer_optional) {
    return false;
  }

  return true;
}

absl::optional<IssuerInfo> GetIssuerForType(const IssuersInfo& issuers,
                                            const IssuerType issuer_type) {
  const auto iter = std::find_if(issuers.issuers.begin(), issuers.issuers.end(),
                                 [issuer_type](const IssuerInfo& issuer) {
                                   return issuer.type == issuer_type;
                                 });
  if (iter == issuers.issuers.end()) {
    return absl::nullopt;
  }

  return *iter;
}

bool PublicKeyExistsForIssuerType(const IssuerType issuer_type,
                                  const std::string& public_key) {
  const IssuersInfo& issuers = GetIssuers();

  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, issuer_type);
  if (!issuer_optional) {
    return false;
  }
  const IssuerInfo& issuer = issuer_optional.value();

  return PublicKeyExists(issuer, public_key);
}

}  // namespace ads
