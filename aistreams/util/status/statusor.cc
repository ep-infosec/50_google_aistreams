// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/util/status/statusor.h"

#include "absl/base/attributes.h"
#include "aistreams/port/logging.h"
#include "aistreams/util/status/canonical_errors.h"
#include "aistreams/util/status/status.h"

namespace aistreams {
namespace internal_statusor {

void Helper::HandleInvalidStatusCtorArg(::aistreams::Status* status) {
  const char* kMessage =
      "An OK status is not a valid constructor argument to StatusOr<T>";
  LOG(ERROR) << kMessage;
  *status = ::aistreams::InternalError(kMessage);
}

void Helper::Crash(const ::aistreams::Status& status) {
  LOG(FATAL) << "Attempting to fetch value instead of handling error "
             << status;
}

}  // namespace internal_statusor
}  // namespace aistreams
