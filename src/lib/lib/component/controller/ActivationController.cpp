#include "ActivationController.h"

#include "IApplicationSettings.hpp"
#include "StorageAccess.h"
#include "type/activation/MessageActivateLegend.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/activation/MessageActivateTokens.h"
#include "type/code/MessageChangeFileView.h"
#include "type/code/MessageScrollToLine.h"
#include "type/error/MessageErrorsAll.h"
#include "type/MessageFlushUpdates.h"
#include "type/MessageRefreshUI.h"
#include "type/MessageStatus.h"
#include "type/MessageTooltipShow.h"
#include "utility.h"

ActivationController::ActivationController(StorageAccess* storageAccess) : m_storageAccess(storageAccess) {}

void ActivationController::clear() {}

void ActivationController::handleMessage(MessageActivateEdge* message) {
  if(message->isBundledEdges()) {
    MessageActivateTokens messageActivateTokens(message);
    messageActivateTokens.tokenIds = message->bundledEdgesIds;
    messageActivateTokens.setKeepContent(false);
    messageActivateTokens.isBundledEdges = true;
    messageActivateTokens.dispatchImmediately();
  } else {
    MessageActivateTokens messageActivateTokens(message);
    messageActivateTokens.tokenIds.push_back(message->tokenId);
    messageActivateTokens.isEdge = true;
    messageActivateTokens.dispatchImmediately();
  }
}

void ActivationController::handleMessage(MessageActivateFile* message) {
  const Id fileId = m_storageAccess->getNodeIdForFileNode(message->filePath);

  if(fileId != 0U) {
    MessageActivateTokens messageActivateTokens(message);
    messageActivateTokens.tokenIds.push_back(fileId);
    messageActivateTokens.searchMatches = m_storageAccess->getSearchMatchesForTokenIds({fileId});
    messageActivateTokens.dispatchImmediately();
  } else {
    MessageChangeFileView msg(message->filePath,
                              MessageChangeFileView::FILE_MAXIMIZED,
                              MessageChangeFileView::VIEW_CURRENT,
                              CodeScrollParams::toFile(message->filePath, CodeScrollParams::Target::VISIBLE));
    msg.setSchedulerId(message->getSchedulerId());
    msg.dispatchImmediately();
  }

  if(message->line > 0) {
    MessageScrollToLine msg(message->filePath, message->line);
    msg.setSchedulerId(message->getSchedulerId());
    msg.dispatch();
  }
}

void ActivationController::handleMessage(MessageActivateNodes* message) {
  MessageActivateTokens messageActivateTokens(message);
  for(const auto& node : message->nodes) {
    Id nodeId = node.nodeId;
    if(nodeId == 0U) {
      nodeId = m_storageAccess->getNodeIdForNameHierarchy(node.nameHierarchy);
    }

    if(nodeId > 0) {
      messageActivateTokens.tokenIds.push_back(nodeId);
    }
  }
  messageActivateTokens.searchMatches = m_storageAccess->getSearchMatchesForTokenIds(messageActivateTokens.tokenIds);
  messageActivateTokens.dispatchImmediately();
}

void ActivationController::handleMessage(MessageActivateTokenIds* message) {
  MessageActivateTokens messageActivateTokens(message);
  messageActivateTokens.tokenIds = message->tokenIds;
  messageActivateTokens.searchMatches = m_storageAccess->getSearchMatchesForTokenIds(message->tokenIds);
  messageActivateTokens.dispatchImmediately();
}

void ActivationController::handleMessage(MessageActivateSourceLocations* message) {
  MessageActivateNodes messageActivateNodes;
  for(Id const nodeId : m_storageAccess->getNodeIdsForLocationIds(message->locationIds)) {
    messageActivateNodes.addNode(nodeId);
  }

  if(message->containsUnsolvedLocations && messageActivateNodes.nodes.size() == 1 &&
     m_storageAccess->getNameHierarchyForNodeId(messageActivateNodes.nodes[0].nodeId).getQualifiedName() == L"unsolved symbol") {
    MessageTooltipShow messageTooltipShow(message->locationIds, {}, TOOLTIP_ORIGIN_CODE);
    messageTooltipShow.force = true;
    messageTooltipShow.dispatch();
    return;
  }

  messageActivateNodes.setSchedulerId(message->getSchedulerId());
  messageActivateNodes.dispatchImmediately();
}

void ActivationController::handleMessage(MessageResetZoom* /*message*/) {
  auto* settings = IApplicationSettings::getInstanceRaw();
  const int fontSizeStd = settings->getFontSizeStd();

  if(settings->getFontSize() != fontSizeStd) {
    settings->setFontSize(fontSizeStd);
    settings->save();

    MessageRefreshUI().dispatch();
  }

  MessageStatus(L"Font size: " + std::to_wstring(fontSizeStd)).dispatch();
}

void ActivationController::handleMessage(MessageSearch* message) {
  const std::vector<SearchMatch>& matches = message->getMatches();

  if(matches.size() && matches.back().searchType == SearchMatch::SEARCH_COMMAND) {
    switch(matches.back().getCommandType()) {
    case SearchMatch::COMMAND_ALL:
    case SearchMatch::COMMAND_NODE_FILTER: {
      MessageActivateOverview msg(message->acceptedNodeTypes);
      msg.setSchedulerId(message->getSchedulerId());
      msg.dispatch();
      return;
    }

    case SearchMatch::COMMAND_ERROR: {
      MessageErrorsAll msg;
      msg.setSchedulerId(message->getSchedulerId());
      msg.dispatch();
      return;
    }

    case SearchMatch::COMMAND_LEGEND: {
      MessageActivateLegend msg;
      msg.setSchedulerId(message->getSchedulerId());
      msg.dispatch();
      return;
    }
    }
  }

  MessageActivateTokens messageActivateTokens(message);
  messageActivateTokens.isFromSearch = true;
  for(const auto& match : matches) {
    if(match.tokenIds.size() && match.tokenIds[0] != 0) {
      utility::append(messageActivateTokens.tokenIds, match.tokenIds);
      messageActivateTokens.searchMatches.push_back(match);
    }
  }
  messageActivateTokens.dispatchImmediately();
}

void ActivationController::handleMessage(MessageZoom* message) {
  bool zoomIn = message->zoomIn;

  IApplicationSettings* settings = IApplicationSettings::getInstanceRaw();

  int fontSize = settings->getFontSize();
  int maxSize = settings->getFontSizeMax();
  int minSize = settings->getFontSizeMin();

  if((fontSize >= maxSize && zoomIn) || (fontSize <= minSize && !zoomIn)) {
    return;
  }

  settings->setFontSize(fontSize + (message->zoomIn ? 1 : -1));
  settings->save();

  MessageStatus(L"Font size: " + std::to_wstring(settings->getFontSize())).dispatch();
  MessageRefreshUI().dispatch();
}
