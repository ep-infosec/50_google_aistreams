/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// TODO(dschao): Can remove this when absl releases Status.

#ifndef AISTREAMS_UTIL_STATUS_STATUS_H_
#define AISTREAMS_UTIL_STATUS_STATUS_H_

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "aistreams/port/logging.h"

namespace aistreams {

enum class StatusCode {
  kOk = 0,
  kCancelled = 1,
  kUnknown = 2,
  kInvalidArgument = 3,
  kDeadlineExceeded = 4,
  kNotFound = 5,
  kAlreadyExists = 6,
  kPermissionDenied = 7,
  kResourceExhausted = 8,
  kFailedPrecondition = 9,
  kAborted = 10,
  kOutOfRange = 11,
  kUnimplemented = 12,
  kInternal = 13,
  kUnavailable = 14,
  kDataLoss = 15,
  kUnauthenticated = 16,
  kDoNotUseReservedForFutureExpansionUseDefaultInSwitchInstead_ = 20
};

#if defined(__clang__)
// Only clang supports warn_unused_result as a type annotation.
class ABSL_MUST_USE_RESULT Status;
#endif

// Streams StatusCodeToString(code) to `os`.
std::ostream& operator<<(std::ostream& os, StatusCode code);

// Denotes success or failure of a call.
class Status {
 public:
  // Creates a success status.
  Status() {}

  // Creates a status with the specified error code and msg as a
  // human-readable std::string containing more detailed information.
  Status(::aistreams::StatusCode code, absl::string_view msg);

  // Copies the specified status.
  Status(const Status& s);
  Status& operator=(const Status& s);

  // Returns true iff the status indicates success.
  bool ok() const {
    return (state_ == NULL) || (state_->code == ::aistreams::StatusCode::kOk);
  }

  ::aistreams::StatusCode code() const {
    return ok() ? ::aistreams::StatusCode::kOk : state_->code;
  }

  const std::string& error_message() const {
    return ok() ? empty_string() : state_->msg;
  }

  absl::string_view message() const {
    return absl::string_view(error_message());
  }

  bool operator==(const Status& x) const;
  bool operator!=(const Status& x) const;

  // If `ok()`, stores `new_status` into `*this`.  If `!ok()`,
  // preserves the current status, but may augment with additional
  // information about `new_status`.
  //
  // Convenient way of keeping track of the first error encountered.
  // Instead of:
  //   `if (overall_status.ok()) overall_status = new_status`
  // Use:
  //   `overall_status.Update(new_status);`
  void Update(const Status& new_status);

  // Returns a std::string representation of this status suitable for
  // printing. Returns the std::string `"OK"` for success.
  std::string ToString() const;

  // Ignores any errors. This method does nothing except potentially suppress
  // complaints from any tools that are checking that errors are not dropped on
  // the floor.
  void IgnoreError() const;

 private:
  static const std::string& empty_string();
  struct State {
    ::aistreams::StatusCode code;
    std::string msg;
  };
  // OK status has a `NULL` state_.  Otherwise, `state_` points to
  // a `State` structure containing the error code and message(s)
  std::unique_ptr<State> state_;

  void SlowCopyFrom(const State* src);
};

inline Status::Status(const Status& s)
    : state_((s.state_ == NULL) ? NULL : new State(*s.state_)) {}

inline Status& Status::operator=(const Status& s) {
  // The following condition catches both aliasing (when this == &s),
  // and the common case where both s and *this are ok.
  if (state_ != s.state_) {
    SlowCopyFrom(s.state_.get());
  }
  return *this;
}

inline bool Status::operator==(const Status& x) const {
  return (this->state_ == x.state_) || (ToString() == x.ToString());
}

inline bool Status::operator!=(const Status& x) const { return !(*this == x); }

inline Status OkStatus() { return Status(); }

std::ostream& operator<<(std::ostream& os, const Status& x);

typedef std::function<void(const Status&)> StatusCallback;

extern std::string* CheckOpHelperOutOfLine(const ::aistreams::Status& v,
                                           const char* msg);

inline std::string* CheckOpHelper(::aistreams::Status v, const char* msg) {
  if (v.ok()) return nullptr;
  return CheckOpHelperOutOfLine(v, msg);
}

#define DO_CHECK_OK(val, level)                                \
  while (auto _result = ::aistreams::CheckOpHelper(val, #val)) \
  LOG(level) << *(_result)

#define CHECK_OK(val) DO_CHECK_OK(val, FATAL)
#define QCHECK_OK(val) DO_CHECK_OK(val, QFATAL)

#ifndef NDEBUG
#define DCHECK_OK(val) CHECK_OK(val)
#else
#define DCHECK_OK(val) \
  while (false && (::aistreams::OkStatus() == (val))) LOG(FATAL)
#endif

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_STATUS_STATUS_H_
