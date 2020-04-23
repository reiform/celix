/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef MSG_ENDPOINT_H
#define MSG_ENDPOINT_H

#include <stdint.h>

#define MSG_ENDPOINT_NAME "msg_endpoint"

typedef struct msgEndPoint {
  uint32_t sync;
  uint32_t msgId;
  uint32_t seqNr;
} msgEndPoint_t;

#endif //MSG_ENDPOINT_H
