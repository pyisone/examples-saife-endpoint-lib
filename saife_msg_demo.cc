/*
 * Copyright (c) 2014 SAIFE Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING, THE SOFTWARE
 * AND DOCUMENTATION ARE DISTRIBUTED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT.  REFER TO THE WRITTEN AGREEMENT FOR SPECIFIC
 * LANGUAGE GOVERNING PERMISSIONS AND LIMITATIONS.
 *
 * Created on: Aug 28, 2014
 * Author:  Pyisone Win
 *
 */

#include <iostream>
#include <thread>
#include <termios.h>
#include <unistd.h>

#include <saife/saife_exception.h>
#include <saife/distinguished_name.h>
#include <saife/no_such_contact_exception.h>
#include <saife/saife_address.h>
#include <saife/saife_interface.h>
#include <saife/saife_factory.h>
#include <saife/log_sink_factory.h>
#include <saife/log_sink_interface.h>
#include <saife/log_sink_manager_interface.h>
#include <saife/saife_management_credentials.h>
#include <saife/saife_io_exception.h>
#include <saife/saife_messaging_interface.h>

using saife::SaifeException;
using saife::SaifeAddress;
using saife::SaifeContact;
using saife::DistinguishedName;
using saife::NoSuchContactException;
using saife::SaifeFactory;
using saife::SaifeInterface;
using saife::LogSinkFactory;
using saife::LogSinkInterface;
using saife::LogSinkManagerInterface;
using saife::SaifeManagementState;
using saife::SaifeManagementCredentials;
using saife::SaifeIoException;
using saife::SaifeMessagingInterface;

static const char* kUidAddressType = "testapp:UID";
static const char* kMessageType = "secret:SensorMessage";
std::string alias_name;

// The SAIFE logging framework provides default loggers for logging to console and
// logging to a file. For this example we will show loggingto the console.

LogSinkFactory log_factory;

LogSinkManagerInterface* log_receiver_ptr = log_factory.CreateConsoleSink();

// Create theSAIFE instance
SaifeFactory factory;
SaifeInterface* saife_ptr = factory.ConstructLocalSaife(log_receiver_ptr);

//This class is to check any awaiting message for the device.
//If message(s) is/are waiting, get the message and print out.
//It will clear the memory allocation for the store message after message is fully consumed.

class get_message {
 public:
  void check_message() {
    do {
      //check message
      std::vector<SaifeMessagingInterface::SaifeMessageData*> msg_ptr;
      saife_ptr->GetMessages(kMessageType, &msg_ptr);
      if (msg_ptr.size() != 0) {
        //get messages
        for (std::vector<SaifeMessagingInterface::SaifeMessageData*>::iterator iter = msg_ptr.begin();
            iter != msg_ptr.end(); ++iter) {
          SaifeMessagingInterface::SaifeMessageData *msgPtr = *iter;
          std::cout << "From: " << msgPtr->sender.alias() << ":::::>"
              << std::string(msgPtr->message_bytes.begin(), msgPtr->message_bytes.end()) << std::endl;
          delete (*iter);
          *iter = 0;
        }
        msg_ptr.clear();
      }
      sleep(1);
    } while (true);
  }
};

//Sending a message to another contact by choosing the contact alias

class send_message {
 public:
  void create_message() {

    do {

      std::string text_base_msg;
      std::getline(std::cin, text_base_msg);

      const std::vector<uint8_t> msg_bytes(text_base_msg.begin(), text_base_msg.end());
      const uint32_t ttl_secs = 604800;  // One week
      const uint32_t tts_msecs = 30000;  // 30 seconds

      bool msg_sent = false;
      try {
        const std::string app_alias(alias_name);
        SaifeContact contact = saife_ptr->GetContactByAlias(app_alias);

        saife_ptr->SendMessage(msg_bytes, kMessageType, contact, ttl_secs, tts_msecs, msg_sent);

        msg_sent = true;
      } catch (NoSuchContactException& nsce) {
        std::cerr << "No valid contact found to send message to" << std::endl;
        //return (1);
      } catch (SaifeIoException& sioe) {
        std::cerr << "Unable to send message to SAIFE Network. Error = " << sioe.error() << std::endl;
        //return (1);
      } catch (...) {
        std::cerr << "Failed to search for server contact by address with unexpected error" << std::endl;
        //return (1);
      }
      sleep(1);

    } while (true);
  }
};

void echo(bool on = true) {
  struct termios settings;
  tcgetattr(STDIN_FILENO, &settings);
  settings.c_lflag = on ? (settings.c_lflag | ECHO) : (settings.c_lflag & ~(ECHO));
  tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}

int main(int argc, char *argv[]) {

  const std::string store("adapter_store");
  const std::string password("this is my special password");

  // Set the logging level for the SAFIE logging framework
  saife_ptr->SetSaifeLogLevel(LogSinkInterface::SAIFE_LOG_ERROR);

  // Initialize the client
  SaifeManagementState state;
  try {
    state = saife_ptr->Initialize(store);
  } catch (SaifeException& se) {
    std::cerr << "Failed to initialize with error: " << se.error() << std::endl;
    return (1);
  } catch (...) {
    std::cerr << "Failed to initialize library with unexpected error" << std::endl;
    return (1);
  }

  std::cout << "Successfully initialized adapter library. State = " << state << std::endl;

  if (state == saife::SAIFE_UNKEYED) {

    //SAIFE Management URL input
    std::cout << "Enter management URL:";
    std::string url_address;
    std::getline(std::cin, url_address);
    const std::string sms_url = url_address;

    //SAIFE Management domain iput
    std::cout << "Enter management domain:";
    std::string domain_name;
    std::getline(std::cin, domain_name);
    const std::string sms_domain = domain_name;

    //SAIFE Management username iput
    std::cout << "Enter management username:";
    std::string user_name;
    std::getline(std::cin, user_name);
    const std::string sms_user = user_name;

    //SAIFE Management password iput
    std::string usr_password;
    std::cout << "Enter management password:";
    echo(false);
    std::getline(std::cin, usr_password);
    echo(true);
    std::cout << std::endl;
    const std::string sms_password = usr_password;

    const SaifeManagementCredentials sms_creds(sms_url, sms_domain, sms_user, sms_password);

    // The alias chosen here should be unique within the application's namespace. The SAIFE network enforces uniqueness of alias names
    std::cout << "Enter alias name:";
    std::string app_aliasname;
    std::getline(std::cin, app_aliasname);
    const std::string alias(app_aliasname);
    const DistinguishedName dn(alias);

    // The application can define one or more addresses to include as metadata to other SAIFE node
    // instances. An address could be a UID of some kind, an URL/URI, a name, or any other metadata
    // that can be represented as a string.

    std::vector < SaifeAddress > address_list;

    SaifeAddress app_id;
    app_id.set_address("5ZQ4L3kTJhJ56AJSKPqFJtgP8zqV7U9X");  // An example UID. Should really be generated in a way that it is unique for each app instance
    app_id.set_address_type(kUidAddressType);
    address_list.push_back(app_id);

    try {
      saife_ptr->GenerateKeys(dn, password, sms_creds, address_list);
    } catch (SaifeException& se) {
      std::cerr << "Failed to generate keys. Error: " << se.error() << std::endl;
      return (1);
    } catch (...) {
      std::cerr << "Failed to generate keys with unexpected error" << std::endl;
      return (1);
    }

    std::cout << "Successfully generated keys" << std::endl;
  }

  if (saife_ptr->IsUnlocked() == false) {
    try {
      saife_ptr->Unlock(password);
    } catch (SaifeException& se) {
      std::cerr << "Failed to unlock. Error: " << se.error() << std::endl;
      return (1);
    } catch (...) {
      std::cerr << "Failed to unlock with unexpected error" << std::endl;
      return (1);
    }
  }

  std::cout << "Successfully unlocked SAIFE library" << std::endl;

  // Refresh data from SAIFE network
  try {
    saife_ptr->UpdateSaifeData();
  } catch (SaifeException& se) {
    std::cerr << "Failed to sync with SAIFE network. Error: " << se.error() << std::endl;
    return (1);
  } catch (...) {
    std::cerr << "Failed to with SAIFE network with unexpected error" << std::endl;
    return (1);
  }

  std::cout << "Successfully synced with SAIFE Network" << std::endl;

  if (saife_ptr->GetAllContacts().size() == 0) {
    std::cout << "Something wrong!!!You need to have at least one contact to send a message." << std::endl;

    return (1);
  }
  std::cout << "The contact size is: " << saife_ptr->GetAllContacts().size() << std::endl;
  const std::vector<SaifeContact> contact_list = saife_ptr->GetAllContacts();
  for (unsigned i = 0; i < contact_list.size(); i++) {
    SaifeContact contact;
    contact = contact_list[i];
    std::cout << contact.alias() << std::endl;
  }
  std::cout << "Choose Contact:";
  std::getline(std::cin, alias_name);

  saife_ptr->Subscribe();
  do {
    if (saife_ptr->GetSubscriptionState() == saife::SaifeMessagingInterface::SAIFE_SUBSCRIBED_AUTHENTICATED
        || saife_ptr->GetSubscriptionState() == saife::SaifeMessagingInterface::SAIFE_SUBSCRIBED_UNAUTHENTICATED) {
      break;
    }
    sleep(1);
  } while (true);

  std::cout << "To terminate the program Ctrl+C" << std::endl;
  std::cout << "Enter your message to send to your contact" << std::endl;

  std::thread t(&get_message::check_message, get_message());
  std::thread q(&send_message::create_message, send_message());
  q.join();

}
