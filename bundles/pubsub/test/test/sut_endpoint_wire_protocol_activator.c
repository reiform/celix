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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h"
#include "celix_properties.h"
#include <pubsub_constants.h>
#include "celix_api.h"
#include "pubsub_protocol.h"
#include <jansson.h>
#define SUT_WIRE_PROTOCOL_TYPE "sut_wire_protocol_headerless"
static const unsigned int PROTOCOL_WIRE_SYNC = 0xABBABAAB;

typedef struct sut_endpoint_protocol_wire {
} sut_endpoint_protocol_wire_t;

celix_status_t pubsubProtocol_create(sut_endpoint_protocol_wire_t **protocol) {
  celix_status_t status = CELIX_SUCCESS;
  *protocol = calloc(1, sizeof(**protocol));
  if (!*protocol) {
    status = CELIX_ENOMEM;
  }
  return status;
}

celix_status_t pubsubProtocol_destroy(sut_endpoint_protocol_wire_t* protocol) {
  celix_status_t status = CELIX_SUCCESS;
  free(protocol);
  return status;
}

celix_status_t pubsubProtocol_getHeaderSize(void* handle, size_t *length) {
  *length = 52; // header + sync + version = 24
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getHeaderBufferSize(void* handle, size_t *length) {
  // Use no header
  *length = 0;
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getSyncHeaderSize(void* handle,  size_t *length) {
  *length = sizeof(int);
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_getSyncHeader(void* handle, void *syncHeader) {
  unsigned int* data = (unsigned int*)syncHeader;
  *data = 0x0000;
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_isMessageSegmentationSupported(void* handle, bool *isSupported) {
  *isSupported = false;
  return CELIX_SUCCESS;
}
celix_status_t pubsubProtocol_encodeHeader(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
  celix_status_t status = CELIX_SUCCESS;
  // Headerless message
  *outBuffer = NULL;
  *outLength = 0;
  return status;
}

celix_status_t pubsubProtocol_encodePayload(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
  *outBuffer = NULL;
  *outLength = 0;
  json_error_t error;
  // Add message size to payload
  json_t *jsMsg = json_loadb(message->payload.payload, message->payload.length-1, 0, &error);
  if (jsMsg) {
    json_object_set_new_nocheck(jsMsg, "msgSize", json_integer(message->payload.length));
    const char *msg = json_dumps(jsMsg, 0);
    *outBuffer = (void *) msg;
    *outLength = strlen(msg);
  }
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_encodeMetadata(void *handle, pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
  celix_status_t status = CELIX_SUCCESS;
  *outBuffer = NULL;
  *outLength = 0;
  return status;
}

celix_status_t pubsubProtocol_decodeHeader(void* handle, void *data, size_t length, pubsub_protocol_message_t *message) {
  celix_status_t status = CELIX_SUCCESS;
  size_t headerSize = 0;
  pubsubProtocol_getHeaderSize(handle, &headerSize);
  json_error_t error;
  char* msg = (char*)data;
  msg[length -1] = '}';
  msg[length - 2] = '\0';
  msg[length - 3] = '}';
  unsigned int ll = strlen(msg);
  json_t *jsMsg = json_loadb(data, ll, 0, &error);
  if(jsMsg != NULL) {
    json_t *jsSync = json_object_get(jsMsg, "sync");
    json_t *jsMsgId = json_object_get(jsMsg, "msgId");
    json_t *jsMsgSize = json_object_get(jsMsg, "msgSize");
    if (jsSync && jsMsgId && jsMsgSize) {
      unsigned int sync  = (uint32_t) json_integer_value(jsSync);
      unsigned int msgId = (uint32_t) json_integer_value(jsMsgId);
      unsigned int msgSize = (uint32_t) json_integer_value(jsMsgSize);
      if (sync != PROTOCOL_WIRE_SYNC) {
        status = CELIX_ILLEGAL_ARGUMENT;
      } else {
        message->header.msgId = msgId;
        message->header.msgMajorVersion = 0;
        message->header.msgMinorVersion = 0;
        message->header.payloadSize  = msgSize;
        message->header.metadataSize = 0;
        message->header.seqNr           = 0;
        message->header.payloadPartSize = message->header.payloadSize;
        message->header.payloadOffset   = 0;
      }
    }
    json_decref(jsMsg);
  }
  return status;
}

celix_status_t pubsubProtocol_decodePayload(void* handle, void *data, size_t length, pubsub_protocol_message_t *message){
  message->payload.payload = data;
  message->payload.length = length;
  return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_decodeMetadata(void* handle, void *data, size_t length, pubsub_protocol_message_t *message) {
  celix_status_t status = CELIX_SUCCESS;
  return status;
}


typedef struct ps_wp_activator {
    sut_endpoint_protocol_wire_t *wireprotocol;
    pubsub_protocol_service_t protocolSvc;
    long wireProtocolSvcId;
} ps_wp_activator_t;

static int ps_wp_start(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    act->wireProtocolSvcId = -1L;

    celix_status_t status = pubsubProtocol_create(&(act->wireprotocol));
    if (status == CELIX_SUCCESS) {
        /* Set serializertype */
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PROTOCOL_TYPE_KEY, SUT_WIRE_PROTOCOL_TYPE);

        act->protocolSvc.getHeaderSize = pubsubProtocol_getHeaderSize;
        act->protocolSvc.getHeaderBufferSize = pubsubProtocol_getHeaderBufferSize;
        act->protocolSvc.getSyncHeaderSize = pubsubProtocol_getSyncHeaderSize;
        act->protocolSvc.getSyncHeader = pubsubProtocol_getSyncHeader;
        act->protocolSvc.isMessageSegmentationSupported = pubsubProtocol_isMessageSegmentationSupported;
        
        act->protocolSvc.encodeHeader = pubsubProtocol_encodeHeader;
        act->protocolSvc.encodePayload = pubsubProtocol_encodePayload;
        act->protocolSvc.encodeMetadata = pubsubProtocol_encodeMetadata;

        act->protocolSvc.decodeHeader = pubsubProtocol_decodeHeader;
        act->protocolSvc.decodePayload = pubsubProtocol_decodePayload;
        act->protocolSvc.decodeMetadata = pubsubProtocol_decodeMetadata;

        act->wireProtocolSvcId = celix_bundleContext_registerService(ctx, &act->protocolSvc, PUBSUB_PROTOCOL_SERVICE_NAME, props);
    }
    return status;
}

static int ps_wp_stop(ps_wp_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->wireProtocolSvcId);
    act->wireProtocolSvcId = -1L;
    pubsubProtocol_destroy(act->wireprotocol);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(ps_wp_activator_t, ps_wp_start, ps_wp_stop)