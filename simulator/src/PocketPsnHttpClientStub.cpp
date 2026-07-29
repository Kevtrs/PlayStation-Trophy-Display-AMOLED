#include "PocketPsnHttpClientStub.h"

IPocketPsnHttpClient::Response PocketPsnHttpClientStub::post(const std::string& url, const std::string& body,
                                                              const std::string& contentType, int /*timeoutMs*/) {
  lastUrl_ = url;
  lastBody_ = body;
  lastContentType_ = contentType;
  return queued_;
}
