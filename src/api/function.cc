/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "function.h"

namespace EscargotShim {

static v8::internal::Address* ToAddress(HandleWrap** ref) {
  return reinterpret_cast<v8::internal::Address*>(ref);
}

FunctionCallbackInfoWrap::FunctionCallbackInfoWrap(v8::Isolate* isolate,
                                                   ValueRef* holder,
                                                   ValueRef* thisValue,
                                                   int argc,
                                                   ValueRef** argv)
    : v8::FunctionCallbackInfo<v8::Value>(
          ToAddress(m_implicitArgs),
          ToAddress(toWrapperArgs(thisValue, argc, argv)),
          argc) {
  m_implicitArgs[T::kHolderIndex] = ValueWrap::createValue(holder);
  m_implicitArgs[T::kIsolateIndex] = reinterpret_cast<HandleWrap*>(isolate);
}

HandleWrap** FunctionCallbackInfoWrap::toWrapperArgs(ValueRef* thisValue,
                                                     int argc,
                                                     ValueRef** argv) {
  HandleWrap** values = new HandleWrap*[argc + 1];
#ifdef V8_REVERSE_JSARGS
#error "Not implement V8_REVERSE_JSARGS"
#else
  for (int i = 0; i < argc; i++) {
    values[argc - i - 1] = ValueWrap::createValue(argv[i]);
  }
  values[argc] = ValueWrap::createValue(thisValue);
  return values + argc - 1;
#endif
}

FunctionCallbackInfoWrap::~FunctionCallbackInfoWrap() {
  HandleWrap** values =
      reinterpret_cast<HandleWrap**>(this->values_) - this->Length() + 1;
  delete[] values;
}

}  // namespace EscargotShim