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
// The primary use-case for StatusOr<T> is as the return value of a
// function which may fail.
//
// Example client usage for a StatusOr<T>, where T is not a pointer:
//
//  ::aistreams::StatusOr<float> result = DoBigCalculationThatCouldFail();
//  if (result.ok()) {
//    float answer = result.ValueOrDie();
//    printf("Big calculation yielded: %f", answer);
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example client usage for a StatusOr<T*>:
//
//  ::aistreams::StatusOr<Foo*> result = FooFactory::MakeNewFoo(arg);
//  if (result.ok()) {
//    std::unique_ptr<Foo> foo(result.ValueOrDie());
//    foo->DoSomethingCool();
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example client usage for a StatusOr<std::unique_ptr<T>>:
//
//  ::aistreams::StatusOr<std::unique_ptr<Foo>> result =
//    FooFactory::MakeNewFoo(arg);
//  if (result.ok()) {
//    std::unique_ptr<Foo> foo = std::move(result.ValueOrDie());
//    foo->DoSomethingCool();
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example factory implementation returning StatusOr<T*>:
//
//  ::aistreams::StatusOr<Foo*> FooFactory::MakeNewFoo(int arg) {
//    if (arg <= 0) {
//      return ::aistreams::InvalidArgumentError("Arg must be positive");
//    } else {
//      return new Foo(arg);
//    }
//  }
//
// Note that the assignment operators require that destroying the currently
// stored value cannot invalidate the argument; in other words, the argument
// cannot be an alias for the current value, or anything owned by the current
// value.

#ifndef AISTREAMS_UTIL_STATUS_STATUSOR_H_
#define AISTREAMS_UTIL_STATUS_STATUSOR_H_

#include "absl/base/attributes.h"
#include "aistreams/util/status/status.h"
#include "aistreams/util/status/status_builder.h"
#include "aistreams/util/status/statusor_internals.h"

namespace aistreams {

#if defined(__clang__)
// Only clang supports warn_unused_result as a type annotation.
template <typename T>
class ABSL_MUST_USE_RESULT StatusOr;
#endif

template <typename T>
class StatusOr : private internal_statusor::StatusOrData<T>,
                 private internal_statusor::TraitsBase<
                     std::is_copy_constructible<T>::value,
                     std::is_move_constructible<T>::value> {
  template <typename U>
  friend class StatusOr;

  typedef internal_statusor::StatusOrData<T> Base;

 public:
  typedef T element_type;

  // Constructs a new StatusOr with Status::UNKNOWN status.  This is marked
  // 'explicit' to try to catch cases like 'return {};', where people think
  // StatusOr<std::vector<int>> will be initialized with an empty vector,
  // instead of a Status::UNKNOWN status.
  explicit StatusOr();

  // StatusOr<T> will be copy constructible/assignable if T is copy
  // constructible.
  StatusOr(const StatusOr&) = default;
  StatusOr& operator=(const StatusOr&) = default;

  // StatusOr<T> will be move constructible/assignable if T is move
  // constructible.
  StatusOr(StatusOr&&) = default;
  StatusOr& operator=(StatusOr&&) = default;

  // Conversion copy/move constructor, T must be convertible from U.
  // TODO: These should not participate in overload resolution if U
  // is not convertible to T.
  template <typename U>
  StatusOr(const StatusOr<U>& other);
  template <typename U>
  StatusOr(StatusOr<U>&& other);

  // Conversion copy/move assignment operator, T must be convertible from U.
  template <typename U>
  StatusOr& operator=(const StatusOr<U>& other);
  template <typename U>
  StatusOr& operator=(StatusOr<U>&& other);

  // Constructs a new StatusOr with the given value. After calling this
  // constructor, calls to ValueOrDie() will succeed, and calls to status() will
  // return OK.
  //
  // NOTE: Not explicit - we want to use StatusOr<T> as a return type
  // so it is convenient and sensible to be able to do 'return T()'
  // when the return type is StatusOr<T>.
  //
  // REQUIRES: T is copy constructible.
  StatusOr(const T& value);

  // Constructs a new StatusOr with the given non-ok status. After calling
  // this constructor, calls to ValueOrDie() will CHECK-fail.
  //
  // NOTE: Not explicit - we want to use StatusOr<T> as a return
  // value, so it is convenient and sensible to be able to do 'return
  // Status()' when the return type is StatusOr<T>.
  //
  // REQUIRES: !status.ok(). This requirement is DCHECKed.
  // In optimized builds, passing Status::OK() here will have the effect
  // of passing ::aistreams::StatusCode::kInternal as a fallback.
  StatusOr(const ::aistreams::Status& status);
  StatusOr& operator=(const ::aistreams::Status& status);
  StatusOr(const ::aistreams::StatusBuilder& builder);
  StatusOr& operator=(const ::aistreams::StatusBuilder& builder);

  // TODO: Add operator=(T) overloads.

  // Similar to the `const T&` overload.
  //
  // REQUIRES: T is move constructible.
  StatusOr(T&& value);

  // RValue versions of the operations declared above.
  StatusOr(::aistreams::Status&& status);
  StatusOr& operator=(::aistreams::Status&& status);
  StatusOr(::aistreams::StatusBuilder&& builder);
  StatusOr& operator=(::aistreams::StatusBuilder&& builder);

  // Returns this->status().ok()
  bool ok() const { return this->status_.ok(); }

  // Returns a reference to a aistreams status. If this contains a T, then
  // returns Status::OK().
  const ::aistreams::Status& status() const&;
  ::aistreams::Status status() &&;

  // Returns a reference to our current value, or CHECK-fails if !this->ok().
  //
  // Note: for value types that are cheap to copy, prefer simple code:
  //
  //   T value = statusor.ValueOrDie();
  //
  // Otherwise, if the value type is expensive to copy, but can be left
  // in the StatusOr, simply assign to a reference:
  //
  //   T& value = statusor.ValueOrDie();  // or `const T&`
  //
  // Otherwise, if the value type supports an efficient move, it can be
  // used as follows:
  //
  //   T value = std::move(statusor).ValueOrDie();
  //
  // The std::move on statusor instead of on the whole expression enables
  // warnings about possible uses of the statusor object after the move.
  // C++ style guide waiver for ref-qualified overloads granted in cl/143176389
  // See go/ref-qualifiers for more details on such overloads.
  const T& ValueOrDie() const&;
  T& ValueOrDie() &;
  const T&& ValueOrDie() const&&;
  T&& ValueOrDie() &&;

  T ConsumeValueOrDie() { return std::move(ValueOrDie()); }

  // Ignores any errors. This method does nothing except potentially suppress
  // complaints from any tools that are checking that errors are not dropped on
  // the floor.
  void IgnoreError() const;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation details for StatusOr<T>

template <typename T>
StatusOr<T>::StatusOr()
    : Base(::aistreams::Status(::aistreams::StatusCode::kUnknown, "")) {}

template <typename T>
StatusOr<T>::StatusOr(const T& value) : Base(value) {}

template <typename T>
StatusOr<T>::StatusOr(const ::aistreams::Status& status) : Base(status) {}

template <typename T>
StatusOr<T>::StatusOr(const ::aistreams::StatusBuilder& builder)
    : Base(builder) {}

template <typename T>
StatusOr<T>& StatusOr<T>::operator=(const ::aistreams::Status& status) {
  this->Assign(status);
  return *this;
}

template <typename T>
StatusOr<T>& StatusOr<T>::operator=(const ::aistreams::StatusBuilder& builder) {
  return *this = static_cast<::aistreams::Status>(builder);
}

template <typename T>
StatusOr<T>::StatusOr(T&& value) : Base(std::move(value)) {}

template <typename T>
StatusOr<T>::StatusOr(::aistreams::Status&& status) : Base(std::move(status)) {}

template <typename T>
StatusOr<T>::StatusOr(::aistreams::StatusBuilder&& builder)
    : Base(std::move(builder)) {}

template <typename T>
StatusOr<T>& StatusOr<T>::operator=(::aistreams::Status&& status) {
  this->Assign(std::move(status));
  return *this;
}

template <typename T>
StatusOr<T>& StatusOr<T>::operator=(::aistreams::StatusBuilder&& builder) {
  return *this = static_cast<::aistreams::Status>(std::move(builder));
}

template <typename T>
template <typename U>
inline StatusOr<T>::StatusOr(const StatusOr<U>& other)
    : Base(static_cast<const typename StatusOr<U>::Base&>(other)) {}

template <typename T>
template <typename U>
inline StatusOr<T>& StatusOr<T>::operator=(const StatusOr<U>& other) {
  if (other.ok())
    this->Assign(other.ValueOrDie());
  else
    this->Assign(other.status());
  return *this;
}

template <typename T>
template <typename U>
inline StatusOr<T>::StatusOr(StatusOr<U>&& other)
    : Base(static_cast<typename StatusOr<U>::Base&&>(other)) {}

template <typename T>
template <typename U>
inline StatusOr<T>& StatusOr<T>::operator=(StatusOr<U>&& other) {
  if (other.ok()) {
    this->Assign(std::move(other).ValueOrDie());
  } else {
    this->Assign(std::move(other).status());
  }
  return *this;
}

template <typename T>
const ::aistreams::Status& StatusOr<T>::status() const& {
  return this->status_;
}
template <typename T>
::aistreams::Status StatusOr<T>::status() && {
  return ok() ? ::aistreams::OkStatus() : std::move(this->status_);
}

template <typename T>
const T& StatusOr<T>::ValueOrDie() const& {
  this->EnsureOk();
  return this->data_;
}

template <typename T>
T& StatusOr<T>::ValueOrDie() & {
  this->EnsureOk();
  return this->data_;
}

template <typename T>
const T&& StatusOr<T>::ValueOrDie() const&& {
  this->EnsureOk();
  return std::move(this->data_);
}

template <typename T>
T&& StatusOr<T>::ValueOrDie() && {
  this->EnsureOk();
  return std::move(this->data_);
}

template <typename T>
void StatusOr<T>::IgnoreError() const {
  // no-op
}

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_STATUS_STATUSOR_H_
