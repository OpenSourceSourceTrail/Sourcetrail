#include "QtRequest.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "logging.h"

QtRequest::QtRequest() {
  m_networkManager = new QNetworkAccessManager(this);
  QObject::connect(m_networkManager, &QNetworkAccessManager::finished, this, &QtRequest::finished);
}

void QtRequest::sendRequest(const QString& url) {
  LOG_INFO(fmt::format("send HTTP request: {}", url.toStdString()));

  try {
    QNetworkRequest request;
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    request.setUrl(QUrl(url));
    m_networkManager->get(request);
  } catch(...) {
    LOG_ERROR("Exception thrown while processing HTTP request.");
    QByteArray bytes;
    emit receivedData(bytes);
  }
}

void QtRequest::finished(QNetworkReply* reply) {
  QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
  QVariant redirectionTargetUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

  Q_UNUSED(statusCodeV);
  Q_UNUSED(redirectionTargetUrl);

  if(reply->error() != QNetworkReply::NoError) {
    LOG_ERROR(fmt::format("An error occurred during http request. ERRORCODE: {}", static_cast<int>(reply->error())));
  }

  QByteArray bytes = reply->readAll();
  LOG_INFO(fmt::format("received HTTP reply: {}", bytes.toStdString()));

  reply->deleteLater();

  emit receivedData(bytes);
}
