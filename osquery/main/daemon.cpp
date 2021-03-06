/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
 
#include <boost/thread.hpp>

#include <glog/logging.h>

#include <osquery/config.h>
#include <osquery/config/plugin.h>
#include <osquery/core.h>
#include <osquery/database.h>
#include <osquery/events.h>
#include <osquery/logger/plugin.h>
#include <osquery/scheduler.h>

int main(int argc, char* argv[]) {
  osquery::initOsquery(argc, argv, osquery::OSQUERY_TOOL_DAEMON);

  auto pid_status = osquery::createPidFile();
  if (!pid_status.ok()) {
    LOG(ERROR) << "Could not create osquery pidfile: " << pid_status.toString();
    ::exit(EXIT_FAILURE);
  }

  try {
    osquery::DBHandle::getInstance();
  } catch (std::exception& e) {
    LOG(ERROR) << "osqueryd failed to start: " << e.what();
    ::exit(1);
  }

  LOG(INFO) << "Listing all plugins";

  LOG(INFO) << "Logger plugins:";
  for (const auto& it : REGISTERED_LOGGER_PLUGINS) {
    LOG(INFO) << "  - " << it.first;
  }

  LOG(INFO) << "Config plugins:";
  for (const auto& it : REGISTERED_CONFIG_PLUGINS) {
    LOG(INFO) << "  - " << it.first;
  }

  LOG(INFO) << "Event Publishers:";
  for (const auto& it : REGISTERED_EVENTPUBLISHERS) {
    LOG(INFO) << "  - " << it.first;
  }

  LOG(INFO) << "Event Subscribers:";
  for (const auto& it : REGISTERED_EVENTSUBSCRIBERS) {
    LOG(INFO) << "  - " << it.first;
  }

  // Start a thread for each appropriate event type
  osquery::registries::faucet(REGISTERED_EVENTPUBLISHERS,
                              REGISTERED_EVENTSUBSCRIBERS);
  osquery::EventFactory::delay();

  boost::thread scheduler_thread(osquery::initializeScheduler);
  scheduler_thread.join();

  // End any event type run loops.
  osquery::EventFactory::end();

  return 0;
}
