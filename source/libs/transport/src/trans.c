/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef USE_UV

#include "transComm.h"

void* (*taosInitHandle[])(uint32_t ip, uint32_t port, char* label, int numOfThreads, void* fp, void* shandle) = {
    taosInitServer, taosInitClient};
void (*taosCloseHandle[])(void* arg) = {taosCloseServer, taosCloseClient};

void* rpcOpen(const SRpcInit* pInit) {
  SRpcInfo* pRpc = calloc(1, sizeof(SRpcInfo));
  if (pRpc == NULL) {
    return NULL;
  }
  if (pInit->label) {
    tstrncpy(pRpc->label, pInit->label, strlen(pInit->label));
  }
  pRpc->cfp = pInit->cfp;
  if (pInit->connType == TAOS_CONN_SERVER) {
    pRpc->numOfThreads = pInit->numOfThreads > TSDB_MAX_RPC_THREADS ? TSDB_MAX_RPC_THREADS : pInit->numOfThreads;
  } else {
    pRpc->numOfThreads = pInit->numOfThreads;
  }
  pRpc->connType = pInit->connType;
  pRpc->idleTime = pInit->idleTime;
  pRpc->tcphandle = (*taosInitHandle[pRpc->connType])(0, pInit->localPort, pRpc->label, pRpc->numOfThreads, NULL, pRpc);
  pRpc->parent = pInit->parent;
  if (pInit->user) {
    memcpy(pRpc->user, pInit->user, strlen(pInit->user));
  }
  if (pInit->secret) {
    memcpy(pRpc->secret, pInit->secret, strlen(pInit->secret));
  }

  return pRpc;
}
void rpcClose(void* arg) {
  SRpcInfo* pRpc = (SRpcInfo*)arg;
  (*taosCloseHandle[pRpc->connType])(pRpc->tcphandle);
  free(pRpc);
  return;
}
void* rpcMallocCont(int contLen) {
  int size = contLen + TRANS_MSG_OVERHEAD;

  char* start = (char*)calloc(1, (size_t)size);
  if (start == NULL) {
    tError("failed to malloc msg, size:%d", size);
    return NULL;
  } else {
    tTrace("malloc mem:%p size:%d", start, size);
  }
  return start + sizeof(STransMsgHead);
}
void rpcFreeCont(void* cont) {
  // impl
  if (cont == NULL) {
    return;
  }
  free((char*)cont - TRANS_MSG_OVERHEAD);
}
void* rpcReallocCont(void* ptr, int contLen) {
  if (ptr == NULL) {
    return rpcMallocCont(contLen);
  }
  char* st = (char*)ptr - TRANS_MSG_OVERHEAD;
  int   sz = contLen + TRANS_MSG_OVERHEAD;
  st = realloc(st, sz);
  if (st == NULL) {
    return NULL;
  }
  return st + TRANS_MSG_OVERHEAD;
}

void rpcSendRedirectRsp(void* thandle, const SEpSet* pEpSet) {
  SRpcMsg rpcMsg;
  memset(&rpcMsg, 0, sizeof(rpcMsg));

  rpcMsg.contLen = sizeof(SEpSet);
  rpcMsg.pCont = rpcMallocCont(rpcMsg.contLen);
  if (rpcMsg.pCont == NULL) return;

  memcpy(rpcMsg.pCont, pEpSet, sizeof(SEpSet));

  rpcMsg.code = TSDB_CODE_RPC_REDIRECT;
  rpcMsg.handle = thandle;

  rpcSendResponse(&rpcMsg);
}

int  rpcReportProgress(void* pConn, char* pCont, int contLen) { return -1; }
void rpcCancelRequest(int64_t rid) { return; }

int32_t rpcInit(void) {
  // impl later
  return 0;
}

void rpcCleanup(void) {
  // impl later
  //
  return;
}
#endif