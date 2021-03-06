/**
 * Copyright 2004-present, Facebook, Inc.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.facebook.profilo.writer;

import com.facebook.proguard.annotations.DoNotStrip;

@DoNotStrip
public interface NativeTraceWriterCallbacks {

  @DoNotStrip
  void onTraceWriteStart(long traceId, int flags);

  @DoNotStrip
  void onTraceWriteEnd(long traceId);

  @DoNotStrip
  void onTraceWriteAbort(long traceId, int abortReason);

  @DoNotStrip
  void onTraceWriteException(long traceId, Throwable t);
}
