/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2025 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See https://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * TCP Network
 */

#include <visp3/core/vpConfig.h>

// Specific case for UWP to introduce a workaround
// error C4996: 'gethostbyname': Use getaddrinfo() or GetAddrInfoW() instead or define _WINSOCK_DEPRECATED_NO_WARNINGS to disable deprecated API warnings
#if defined(WINRT) || defined(_WIN32)
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#endif

// inet_ntop() not supported on win XP
#ifdef VISP_HAVE_FUNC_INET_NTOP

#include <visp3/core/vpNetwork.h>
#include <visp3/core/vpDebug.h>
BEGIN_VISP_NAMESPACE
vpNetwork::vpNetwork()
  : emitter(), receptor_list(), readFileDescriptor(), socketMax(0), request_list(), max_size_message(999999),
  separator("[*@*]"), beginning("[*start*]"), end("[*end*]"), param_sep("[*|*]"), currentMessageReceived(), tv(),
  tv_sec(0), tv_usec(10), verboseMode(false)
{
  tv.tv_sec = tv_sec;
#ifdef TARGET_OS_IPHONE
  tv.tv_usec = static_cast<int>(tv_usec);
#else
  tv.tv_usec = tv_usec;
#endif

#if defined(_WIN32)
  // Enable the sockets to be used
  // Note that: if we were using "winsock.h" instead of "winsock2.h" we would
  // had to use:  WSAStartup(MAKEWORD(1,0), &WSAData);
  WSADATA WSAData;
  WSAStartup(MAKEWORD(2, 0), &WSAData);
#endif
}

vpNetwork::vpNetwork(const vpNetwork &network)
{
  *this = network;
}

vpNetwork &vpNetwork::operator=(const vpNetwork &network)
{
  emitter = network.emitter;
  receptor_list = network.receptor_list;
  readFileDescriptor = network.readFileDescriptor;
  socketMax = network.socketMax;
  request_list = network.request_list;
  max_size_message = network.max_size_message;
  separator = network.separator;
  beginning = network.beginning;
  end = network.end;
  param_sep = network.param_sep;
  currentMessageReceived = network.currentMessageReceived;
  tv = network.tv;
  tv_sec = network.tv_sec;
  tv_usec = network.tv_usec;
  verboseMode = network.verboseMode;

#if defined(_WIN32)
// Enable the sockets to be used
// Note that: if we were using "winsock.h" instead of "winsock2.h" we would
// had to use:  WSAStartup(MAKEWORD(1,0), &WSAData);
  WSADATA WSAData;
  WSAStartup(MAKEWORD(2, 0), &WSAData);
#endif

  return *this;
}

vpNetwork::~vpNetwork()
{
#if defined(_WIN32)
  WSACleanup();
#endif
}

/*!
  Add a decoding request to the emitter. This request will be used to decode
  the received messages. Each request must have a different id.

  \warning vpRequest is a virtual pure class. It has to be implemented
  according to the way how you want to decode the message received.

  \sa vpNetwork::removeDecodingRequest()

  \param req : Request to add.
*/
void vpNetwork::addDecodingRequest(vpRequest *req)
{
  bool alreadyHas = false;

  for (unsigned int i = 0; i < request_list.size(); i++)
    if (request_list[i]->getId() == req->getId()) {
      alreadyHas = true;
      break;
    }

  if (alreadyHas)
    std::cout << "Server already has one request with the similar ID. "
    "Request hasn't been added."
    << std::endl;
  else
    request_list.push_back(req);
}

/*!
  Delete a decoding request from the emitter.

  \sa vpNetwork::addDecodingRequest()

  \param id : Id of the request to delete.
*/
void vpNetwork::removeDecodingRequest(const char *id)
{
  for (unsigned int i = 0; i < request_list.size(); i++) {
    if (request_list[i]->getId() == id) {
      request_list.erase(request_list.begin() + static_cast<int>(i));
      break;
    }
  }
}

/*!
  Print the receptors.

  \param id : Message to display before the receptor's index.
*/
void vpNetwork::print(const char *id)
{
  for (unsigned int i = 0; i < receptor_list.size(); i++) {
    std::cout << id << i << " : " << inet_ntoa(receptor_list[i].receptorAddress.sin_addr) << std::endl;
  }
}

/*!
  Get the receptor index from its name. The name can be either the IP, or its
  name on the network.

  \param name : Name of the receptor.

  \return Index of the receptor, or -1 if an error occurs.
*/
int vpNetwork::getReceptorIndex(const char *name)
{
  struct hostent *server = gethostbyname(name);

  if (server == nullptr) {
    std::string noSuchHostMessage("ERROR, ");
    noSuchHostMessage.append(name);
    noSuchHostMessage.append(": no such host\n");
    vpERROR_TRACE(noSuchHostMessage.c_str(), "vpNetwork::getReceptorIndex()");
    return -1;
  }

  std::string ip = inet_ntoa(*(in_addr *)server->h_addr);

  for (int i = 0; i < static_cast<int>(receptor_list.size()); i++) {
    if (receptor_list[static_cast<unsigned int>(i)].receptorIP == ip)
      return i;
  }

  return -1;
}

/*!
  Send a request to the first receptor in the list.

  \sa vpNetwork::sendRequestTo()
  \sa vpNetwork::sendAndEncodeRequest()
  \sa vpNetwork::sendAndEncodeRequestTo()
  \sa vpNetwork::send()
  \sa vpNetwork::sendTo()

  \param req : Request to send.

  \return The number of bytes that have been sent, -1 if an error occurred.
*/
int vpNetwork::sendRequest(vpRequest &req) { return sendRequestTo(req, 0); }

/*!
  Send a request to a specific receptor.

  \sa vpNetwork::sendRequest()
  \sa vpNetwork::sendAndEncodeRequest()
  \sa vpNetwork::sendAndEncodeRequestTo()
  \sa vpNetwork::send()
  \sa vpNetwork::sendTo()

  \param req : Request to send.
  \param dest : Index of the receptor receiving the request.

  \return The number of bytes that have been sent, -1 if an error occurred.
*/
int vpNetwork::sendRequestTo(vpRequest &req, const unsigned int &dest)
{
  int size = static_cast<int>(receptor_list.size());
  int sizeMinusOne = static_cast<int>(receptor_list.size() - 1);
  if (size == 0 || dest > static_cast<unsigned int>(sizeMinusOne)) {
    if (verboseMode)
      vpTRACE("Cannot Send Request! Bad Index");
    return 0;
  }

  std::string message = beginning + req.getId() + separator;

  if (req.size() != 0) {
    message += req[0];

    for (unsigned int i = 1; i < req.size(); i++) {
      message += param_sep + req[i];
    }
  }

  message += end;

  int flags = 0;
//#if ! defined(APPLE) && ! defined(SOLARIS) && ! defined(_WIN32)
#if defined(__linux__)
  flags = MSG_NOSIGNAL; // Only for Linux
#endif

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
  int value = static_cast<int>(sendto(receptor_list[dest].socketFileDescriptorReceptor, message.c_str(), message.size(), flags,
                                      (sockaddr *)&receptor_list[dest].receptorAddress, receptor_list[dest].receptorAddressSize));
#else
  int value = sendto(static_cast<unsigned int>(receptor_list[dest].socketFileDescriptorReceptor), message.c_str(), static_cast<int>(message.size()),
                     flags, (sockaddr *)&receptor_list[dest].receptorAddress, receptor_list[dest].receptorAddressSize);
#endif

  return value;
}

/*!
  Send and encode a request to the first receptor in the list.

  \sa vpNetwork::sendRequestTo()
  \sa vpNetwork::sendAndEncodeRequest()
  \sa vpNetwork::sendAndEncodeRequestTo()
  \sa vpNetwork::send()
  \sa vpNetwork::sendTo()

  \param req : Request to send.

  \return The number of bytes that have been sent, -1 if an error occurred.
*/
int vpNetwork::sendAndEncodeRequest(vpRequest &req)
{
  req.encode();
  return sendRequest(req);
}

/*!
  Send and encode a request to a specific receptor.

  \sa vpNetwork::sendRequest()
  \sa vpNetwork::sendAndEncodeRequest()
  \sa vpNetwork::sendAndEncodeRequestTo()
  \sa vpNetwork::send()
  \sa vpNetwork::sendTo()

  \param req : Request to send.
  \param dest : Index of the receptor receiving the request.

  \return The number of bytes that have been sent, -1 if an error occurred.
*/
int vpNetwork::sendAndEncodeRequestTo(vpRequest &req, const unsigned int &dest)
{
  req.encode();
  return sendRequestTo(req, dest);
}

/*!
  Receive requests until there is requests to receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()
*/
std::vector<int> vpNetwork::receiveRequest()
{
  privReceiveRequest();
  return privHandleRequests();
}

/*!
  Receives requests, from a specific emitter, until there is request to
  receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message
*/
std::vector<int> vpNetwork::receiveRequestFrom(const unsigned int &receptorEmitting)
{
  privReceiveRequestFrom(receptorEmitting);
  return privHandleRequests();
}

/*!
  Receives a message once (in the limit of the Maximum message size value).
  This message can represent an entire request or not. Several calls to this
  function might be necessary to get the entire request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::receiveRequestOnce()
{
  privReceiveRequestOnce();
  return privHandleFirstRequest();
}

/*!
  Receives a message once (in the limit of the Maximum message size value),
  from a specific emitter. This message can represent an entire request or
  not. Several calls to this function might be necessary to get the entire
  request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message.

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::receiveRequestOnceFrom(const unsigned int &receptorEmitting)
{
  privReceiveRequestOnceFrom(receptorEmitting);
  return privHandleFirstRequest();
}

/*!
  Receives and decode requests until there is requests to receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()
*/
std::vector<int> vpNetwork::receiveAndDecodeRequest()
{
  std::vector<int> res = receiveRequest();
  for (unsigned int i = 0; i < res.size(); i++)
    if (res[i] != -1)
      request_list[static_cast<unsigned int>(res[i])]->decode();

  return res;
}

/*!
  Receives and decode requests, from a specific emitter, until there is
  request to receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message
*/
std::vector<int> vpNetwork::receiveAndDecodeRequestFrom(const unsigned int &receptorEmitting)
{
  std::vector<int> res = receiveRequestFrom(receptorEmitting);
  for (unsigned int i = 0; i < res.size(); i++) {
    if (res[i] != -1)
      request_list[static_cast<unsigned int>(res[i])]->decode();
  }

  return res;
}

/*!
  Receives a message once (in the limit of the Maximum message size value).
  This message can represent an entire request or not. Several calls to this
  function might be necessary to get the entire request. If it represents an
  entire request, it decodes the request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::receiveAndDecodeRequestOnce()
{
  int res = receiveRequestOnce();
  if (res != -1)
    request_list[static_cast<unsigned int>(res)]->decode();

  return res;
}

/*!
  Receives a message once (in the limit of the Maximum message size value),
  from a specific emitter. This message can represent an entire request or
  not. Several calls to this function might be necessary to get the entire
  request. If it represents an entire request, it decodes the request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message.

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::receiveAndDecodeRequestOnceFrom(const unsigned int &receptorEmitting)
{
  int res = receiveRequestOnceFrom(receptorEmitting);
  if (res != -1)
    request_list[static_cast<unsigned int>(res)]->decode();

  return res;
}

//######## Definition of Template Functions ########
//#                                                #
//##################################################

/*!
  Handle requests until there are requests to handle.

  \warning : This function doesn't decode the requests. If it does handle a
  request that hasn't been ran yet, The request's parameters will be replace.

  \sa vpNetwork::handleFirstRequest()

  \return : The list of index corresponding to the requests that have been
  handled.
*/
std::vector<int> vpNetwork::privHandleRequests()
{
  std::vector<int> resIndex;
  int index = privHandleFirstRequest();

  while (index != -1) {
    resIndex.push_back(index);
    index = privHandleFirstRequest();
  }

  return resIndex;
}

/*!
  Handle the first request in the queue.

  \warning : This function doesn't run the request. If it does handle a
  request that hasn't been ran yet, The request's parameters will be replace.

  \sa vpNetwork::handleRequests()

  \return : The index of the request that has been handled.
*/
int vpNetwork::privHandleFirstRequest()
{
  size_t indStart = currentMessageReceived.find(beginning);
  size_t indSep = currentMessageReceived.find(separator);
  size_t indEnd = currentMessageReceived.find(end);

  if (indStart == std::string::npos && indSep == std::string::npos && indEnd == std::string::npos) {
    if (currentMessageReceived.size() != 0)
      currentMessageReceived.clear();

    if (verboseMode)
      vpTRACE("Incorrect message");

    return -1;
  }

  if (indStart == std::string::npos || indSep == std::string::npos || indEnd == std::string::npos)
    return -1;

  if (indEnd < indStart) {
    if (verboseMode)
      vpTRACE("Incorrect message");
    currentMessageReceived.erase(static_cast<unsigned int>(indStart), indEnd + end.size());
    return -1;
  }

  size_t indStart2 = currentMessageReceived.find(beginning, indStart + 1);
  if (indStart2 != std::string::npos && indStart2 < indEnd) {
    if (verboseMode)
      vpTRACE("Incorrect message");
    currentMessageReceived.erase(static_cast<unsigned int>(indStart), static_cast<unsigned int>(indStart2));
    return -1;
  }

  size_t deb = indStart + beginning.size();
  std::string id = currentMessageReceived.substr(static_cast<unsigned int>(deb), indSep - deb);

  int indRequest = 0;
  bool hasBeenFound = false;
  for (unsigned int i = 0; i < request_list.size(); i++) {
    if (id == request_list[i]->getId()) {
      hasBeenFound = true;
      request_list[i]->clear();
      indRequest = static_cast<int>(i);
      break;
    }
  }

  if (!hasBeenFound) {
    // currentMessageReceived.erase(indStart,indEnd+end.size());
    if (verboseMode)
      vpTRACE("No request corresponds to the received message");
    return -1;
  }

  size_t indDebParam = indSep + separator.size();
  size_t indEndParam = currentMessageReceived.find(param_sep, indDebParam);

  std::string param;
  while (indEndParam != std::string::npos || indEndParam < indEnd) {
    param = currentMessageReceived.substr(static_cast<unsigned int>(indDebParam), static_cast<unsigned int>(indEndParam - indDebParam));
    request_list[static_cast<unsigned int>(indRequest)]->addParameter(param);
    indDebParam = indEndParam + param_sep.size();
    indEndParam = currentMessageReceived.find(param_sep, indDebParam);
  }

  param = currentMessageReceived.substr(static_cast<unsigned int>(indDebParam), indEnd - indDebParam);
  request_list[static_cast<unsigned int>(indRequest)]->addParameter(param);
  currentMessageReceived.erase(indStart, indEnd + end.size());

  return indRequest;
}

/*!
  Receive requests until there is requests to receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()
*/
void vpNetwork::privReceiveRequest()
{
  while (privReceiveRequestOnce() > 0) {
  }
}

/*!
  Receives requests, from a specific emitter, until there is request to
  receive.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message
*/
void vpNetwork::privReceiveRequestFrom(const unsigned int &receptorEmitting)
{
  while (privReceiveRequestOnceFrom(receptorEmitting) > 0) {
  }
}

/*!
  Receives a message once (in the limit of the Maximum message size value).
  This message can represent an entire request or not. Several calls to this
  function might be necessary to get the entire request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnceFrom()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::privReceiveRequestOnce()
{
  if (receptor_list.size() == 0) {
    if (verboseMode)
      vpTRACE("No Receptor!");
    return -1;
  }

  tv.tv_sec = tv_sec;
#ifdef TARGET_OS_IPHONE
  tv.tv_usec = static_cast<int>(tv_usec);
#else
  tv.tv_usec = tv_usec;
#endif

  FD_ZERO(&readFileDescriptor);

  for (unsigned int i = 0; i < receptor_list.size(); i++) {
    if (i == 0)
      socketMax = receptor_list[i].socketFileDescriptorReceptor;

    FD_SET(static_cast<unsigned int>(receptor_list[i].socketFileDescriptorReceptor), &readFileDescriptor);
    if (socketMax < receptor_list[i].socketFileDescriptorReceptor)
      socketMax = receptor_list[i].socketFileDescriptorReceptor;
  }

  int value = select(static_cast<int>(socketMax) + 1, &readFileDescriptor, nullptr, nullptr, &tv);
  int numbytes = 0;

  if (value == -1) {
    if (verboseMode)
      vpERROR_TRACE("Select error");
    return -1;
  }
  else if (value == 0) {
 // Timeout
    return 0;
  }
  else {
    for (unsigned int i = 0; i < receptor_list.size(); i++) {
      if (FD_ISSET(static_cast<unsigned int>(receptor_list[i].socketFileDescriptorReceptor), &readFileDescriptor)) {
        char *buf = new char[max_size_message];
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
        numbytes = static_cast<int>(recv(receptor_list[i].socketFileDescriptorReceptor, buf, max_size_message, 0));
#else
        numbytes = recv(static_cast<unsigned int>(receptor_list[i].socketFileDescriptorReceptor), buf, static_cast<int>(max_size_message), 0);
#endif

        if (numbytes <= 0) {
          std::cout << "Disconnected : " << inet_ntoa(receptor_list[i].receptorAddress.sin_addr) << std::endl;
          receptor_list.erase(receptor_list.begin() + static_cast<int>(i));
          delete[] buf;
          return numbytes;
        }
        else {
          std::string returnVal(buf, static_cast<unsigned int>(numbytes));
          currentMessageReceived.append(returnVal);
        }
        delete[] buf;
        break;
      }
    }
  }

  return numbytes;
}

/*!
  Receives a message once (in the limit of the Maximum message size value),
  from a specific emitter. This message can represent an entire request or
  not. Several calls to this function might be necessary to get the entire
  request.

  \warning Requests will be received but not decoded.

  \sa vpNetwork::receive()
  \sa vpNetwork::receiveRequestFrom()
  \sa vpNetwork::receiveRequest()
  \sa vpNetwork::receiveRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequest()
  \sa vpNetwork::receiveAndDecodeRequestFrom()
  \sa vpNetwork::receiveAndDecodeRequestOnce()
  \sa vpNetwork::receiveAndDecodeRequestOnceFrom()

  \param receptorEmitting : Index of the receptor emitting the message.

  \return The number of bytes received, -1 if an error occurred.
*/
int vpNetwork::privReceiveRequestOnceFrom(const unsigned int &receptorEmitting)
{
  int size = static_cast<int>(receptor_list.size());
  int sizeMinusOne = static_cast<int>(receptor_list.size()) - 1;
  if (size == 0 || receptorEmitting > static_cast<unsigned int>(sizeMinusOne)) {
    if (verboseMode)
      vpTRACE("No receptor at the specified index!");
    return -1;
  }

  tv.tv_sec = tv_sec;
#ifdef TARGET_OS_IPHONE
  tv.tv_usec = static_cast<int>(tv_usec);
#else
  tv.tv_usec = tv_usec;
#endif

  FD_ZERO(&readFileDescriptor);

  socketMax = receptor_list[receptorEmitting].socketFileDescriptorReceptor;
  FD_SET(static_cast<unsigned int>(receptor_list[receptorEmitting].socketFileDescriptorReceptor), &readFileDescriptor);

  int value = select(static_cast<int>(socketMax) + 1, &readFileDescriptor, nullptr, nullptr, &tv);
  int numbytes = 0;
  if (value == -1) {
    if (verboseMode)
      vpERROR_TRACE("Select error");
    return -1;
  }
  else if (value == 0) {
 // Timeout
    return 0;
  }
  else {
    if (FD_ISSET(static_cast<unsigned int>(receptor_list[receptorEmitting].socketFileDescriptorReceptor), &readFileDescriptor)) {
      char *buf = new char[max_size_message];
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
      numbytes = static_cast<int>(recv(receptor_list[receptorEmitting].socketFileDescriptorReceptor, buf, max_size_message, 0));
#else
      numbytes = recv(static_cast<unsigned int>(receptor_list[receptorEmitting].socketFileDescriptorReceptor), buf,
                      static_cast<int>(max_size_message), 0);
#endif
      if (numbytes <= 0) {
        std::cout << "Disconnected : " << inet_ntoa(receptor_list[receptorEmitting].receptorAddress.sin_addr)
          << std::endl;
        receptor_list.erase(receptor_list.begin() + static_cast<int>(receptorEmitting));
        delete[] buf;
        return numbytes;
      }
      else {
        std::string returnVal(buf, static_cast<unsigned int>(numbytes));
        currentMessageReceived.append(returnVal);
      }
      delete[] buf;
    }
  }

  return numbytes;
}
END_VISP_NAMESPACE
#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_core.a(vpNetwork.cpp.o) has no symbols
void dummy_vpNetwork() { }
#endif
