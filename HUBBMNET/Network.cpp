#include "Network.h"

Network::Network() {
}

Client* findClient(vector<Client> &clients, const string& clientId) {
    for (Client &client : clients) {
        if (client.client_id == clientId) {
            return &client;
        }
    }
    return nullptr;
}

ApplicationLayerPacket* findAppPacket(stack<Packet*> *frame) {
        stack<Packet*> tempFrame;
        ApplicationLayerPacket* appPacket;
        while (!frame->empty()) {
        if (frame->size() == 1) {
            Packet* packet = frame->top();
            appPacket = dynamic_cast<ApplicationLayerPacket*>(packet);
        }
        tempFrame.push(frame->top());
        frame->pop();
    }

    while (!tempFrame.empty()) {
        frame->push(tempFrame.top());
        tempFrame.pop();
    }

    return appPacket;
}


Client* findClientWithMac(vector<Client> &clients, const string& clientMac) {
    for (Client &client : clients) {
        if (client.client_mac == clientMac) {
            return &client;
        }
    }
    return nullptr;
}

string getCurrentTime() {
    time_t currentTime = time(nullptr);
    tm* localTime = localtime(&currentTime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    return buffer;
}

void processMessage(Client* sender, Client* receiver, const string &sender_port, const string &receiver_port,
         string message, vector<Client> &clients) {

    Client* forwarder = findClient(clients, sender->routing_table[receiver->client_id]);

    stack<Packet*> frame;
    frame.push(new ApplicationLayerPacket(0, sender->client_id, receiver->client_id, message));
    frame.push(new TransportLayerPacket(1, sender_port, receiver_port));
    frame.push(new NetworkLayerPacket(2, sender->client_ip, receiver->client_ip));
    frame.push(new PhysicalLayerPacket (3, sender->client_mac, forwarder->client_mac));

    sender->outgoing_queue.push(frame);

    stack<Packet*> tempStack = frame;

    while (!tempStack.empty()) {
        Packet* currentPacket = tempStack.top();
        currentPacket->print();
        tempStack.pop();
    }

    cout << "Message chunk carried: \"" << message << '\"' << endl;
    cout << "Number of hops so far: " << 0 << endl;
    cout << "--------" << endl;
}

void Network::process_commands(vector<Client> &clients, vector<string> &commands, int message_limit,
                      const string &sender_port, const string &receiver_port) {
    for (int i = 1; i < commands.size(); i++) {
        string printCommand("Command: " + commands[i]);
        istringstream line(commands[i]);
        string command;
        
        line >> command;

        cout << string(printCommand.length(), '-') << "\n" << printCommand << "\n" << string(printCommand.length(), '-') << endl;

        if (command == "MESSAGE") {
            string senderId, receiverId, message;

            line >> senderId >> receiverId;
            line.ignore(numeric_limits<streamsize>::max(), '#');
            getline(line, message, '#');

            cout << "Message to be sent: \"" << message << "\"\n" << endl;

            Client* sender = findClient(clients, senderId);
            Client* receiver = findClient(clients, receiverId);

            int frameCount = 1;

            for (size_t i = 0; i < message.length(); i += message_limit) {
                cout << "Frame #" << frameCount << endl;
                string splitMessage = message.substr(i, message_limit);
                processMessage(sender, receiver, sender_port, receiver_port, splitMessage, clients);
                frameCount++;
            }

            sender->log_entries.push_back(Log(getCurrentTime(), message,
             frameCount-1, 0, sender->client_id, receiver->client_id, true, ActivityType::MESSAGE_SENT));
        }
        else if (command == "SHOW_FRAME_INFO") {
            string clientId, queueType, frameMessage;
            int frame, currentFrame = 1, layer = 0;
            queue<stack<Packet*>>* originalQueue;
            queue<stack<Packet*>> tempQueue;
            queue<stack<Packet*>> copyQueue;
            stack<Packet*> tempStack;
            bool frameFound = false;

            line >> clientId >> queueType >> frame;

            Client* infoClient = findClient(clients, clientId);

            if (queueType == "in") {
                originalQueue = &infoClient->incoming_queue;
                frameMessage =  "Current Frame #" + to_string(frame) + " on the incoming queue of client " + clientId;
            }
            else {
                originalQueue = &infoClient->outgoing_queue;
                frameMessage = "Current Frame #" + to_string(frame) + " on the outgoing queue of client " + clientId;
            }

            while (!originalQueue->empty()) {
                if (currentFrame == frame) { 
                    cout << frameMessage << endl;
                    stack<Packet*> &currentStack = originalQueue->front();

                    while (!currentStack.empty()) {
                        tempStack.push(move(currentStack.top()));
                        currentStack.pop();
                    }
                    Packet* packet = tempStack.top();
                    ApplicationLayerPacket* appPacket = dynamic_cast<ApplicationLayerPacket*>(packet);

                    cout << "Carried Message: \"" << appPacket->message_data << "\"" << endl;
                    while (!tempStack.empty()) {
                        cout << "Layer " << layer++ << " info: ";
                        tempStack.top()->print();
                        currentStack.push(tempStack.top());
                        tempStack.pop();
                    }
                    PhysicalLayerPacket* physicalPacket = dynamic_cast<PhysicalLayerPacket*>(currentStack.top());
                    cout << "Number of hops so far: " << physicalPacket->hops << endl;
                    frameFound = true;
                    break;
                } else {
                    tempQueue.push(originalQueue->front());
                    originalQueue->pop();
                    currentFrame++;
                }
            }

            if (!frameFound) {
                cout << "No such frame." << endl;
            }

            for (; !originalQueue->empty(); originalQueue->pop()) {
                tempQueue.push(originalQueue->front());
            }

            for (; !tempQueue.empty(); tempQueue.pop()) {
                originalQueue->push(tempQueue.front());
            }
        }
        else if (command == "SHOW_Q_INFO") {
            string clientId, queueType;
            queue<stack<Packet*>>* queue;

            line >> clientId >> queueType;

            Client* infoClient = findClient(clients, clientId);

            if (queueType == "in") {
                queue = &infoClient->incoming_queue;
                cout << "Client " << clientId << " Incoming Queue Status" << endl;
            }
            else {
                queue = &infoClient->outgoing_queue;
                cout << "Client " << clientId << " Outgoing Queue Status" << endl;
            }

            cout << "Current total number of frames: " << queue->size() << endl;

        }
        else if (command == "SEND") {
            for (Client &client : clients) {
                if (!client.outgoing_queue.empty()) {
                    Client* receiver;
                    int frameCount = 1;

                    while (!client.outgoing_queue.empty()) {
                        stack<Packet*> *frame = &client.outgoing_queue.front();
                        receiver = findClient(clients, client.routing_table[findAppPacket(frame)->receiver_ID]);
                        PhysicalLayerPacket* physicalPacket = dynamic_cast<PhysicalLayerPacket*>(frame->top());

                        receiver->incoming_queue.push(*frame);

                        cout << "Client " << client.client_id << " sending frame #" << frameCount++ << " to client " << receiver->client_id << endl;

                        while (!frame->empty()) {
                            Packet* currentPacket = frame->top();
                            currentPacket->print();
                            if (frame->size() == 1) {
                                Packet* packet = frame->top();
                                ApplicationLayerPacket* appPacket = dynamic_cast<ApplicationLayerPacket*>(packet);
                                string message = appPacket->message_data;
                                if (message.back() == '.' || message.back() == '!' || message.back() == '?') frameCount = 1;
                                cout << "Message chunk carried: \"" << appPacket->message_data << "\"" << endl;
                                cout << "Number of hops so far: " << ++physicalPacket->hops << endl;
                            }
                            frame->pop();
                        }
                        client.outgoing_queue.pop();
                        cout << "--------" << endl;
                    }
                }
            }
        }
        else if (command == "RECEIVE") {
            for (Client &client : clients) {
                if (!client.incoming_queue.empty()) {

                    int frameCount = 1;
                    string message = "";
                    bool received = false;
                    bool forwarded = false;
                    bool dropped = false;

                    while (!client.incoming_queue.empty()) {
                        stack<Packet*> *frame = &client.incoming_queue.front();
                        ApplicationLayerPacket* appPacket = findAppPacket(frame);
                        PhysicalLayerPacket* physicalPacket = dynamic_cast<PhysicalLayerPacket*>(frame->top());

                        Client* receiveForwarder = findClientWithMac(clients, physicalPacket->sender_MAC_address);
                        Client* forwarder = findClient(clients, client.routing_table[appPacket->receiver_ID]);
                        int hops = physicalPacket->hops;
                        string senderId = appPacket->sender_ID;
                        string receiverId = appPacket->receiver_ID;
                        char lastChar = appPacket->message_data.back();

                        if (receiverId == client.client_id && !received) {
                            received = true;
                        } else if (receiverId != client.client_id && !forwarder) {
                            cout << "Client " << client.client_id << " receiving frame #" << frameCount << " from client " << receiveForwarder->client_id 
                            << ", but intended for client " << receiverId << ". Forwarding... " << endl;
                            forwarded = true;
                        } else if (receiverId != client.client_id && !forwarded) {
                            cout << "Client " << client.client_id << " receiving a message from client " << receiveForwarder->client_id
                            << ", but intended for client " << receiverId << ". Forwarding... " << endl;
                            forwarded = true;
                        }

                        if (received) {
                            cout << "Client " << client.client_id << " receiving frame #" << frameCount << " from client " << 
                            receiveForwarder->client_id << ", originating from client " << senderId << endl;
                            while(!frame->empty()) {
                                frame->top()->print();
                                if (frame->size() == 1) {
                                    cout << "Message chunk carried: \"" << appPacket->message_data << "\"" << endl;
                                    message += appPacket->message_data;
                                }
                                delete frame->top();
                                frame->pop();
                            }
                            cout << "Number of hops so far: " << hops << endl;
                            cout << "--------" << endl;

                        } else if (forwarded) {
                            if (forwarder) {
                                physicalPacket->sender_MAC_address = client.client_mac;
                                physicalPacket->receiver_MAC_address = forwarder->client_mac;
                                message += findAppPacket(frame)->message_data;

                                cout << "Frame #" << frameCount << " MAC address change: New sender MAC " << physicalPacket->sender_MAC_address <<
                                ", new receiver MAC " << physicalPacket->receiver_MAC_address << endl;

                                client.outgoing_queue.push(*frame);
                            } else {
                                cout << "Error: Unreachable destination. Packets are dropped after " << 
                                hops << " hops!" << endl;
                                dropped = true;
                                while(!frame->empty()) {
                                    delete frame->top();
                                    frame->pop();
                                }
                            }
                        }
                        frameCount++;

                        if (lastChar == '.' || lastChar == '!' || lastChar == '?') {
                            if (received) {
                                cout << "Client " << client.client_id << " received the message \"" << message << "\"" << " from client " 
                                << senderId << "." << endl;

                                client.log_entries.push_back(Log(getCurrentTime(), message, frameCount-1, hops, 
                                senderId, client.client_id, true, ActivityType::MESSAGE_RECEIVED));

                                received = false;
                            } else if (forwarded) {
                                if (dropped) {
                                    client.log_entries.push_back(Log(getCurrentTime(), message, frameCount-1, hops, 
                                    senderId, receiverId, false, ActivityType::MESSAGE_DROPPED));
                                    dropped = false;
                                } else {
                                    client.log_entries.push_back(Log(getCurrentTime(), message, frameCount-1, hops, 
                                    senderId, receiverId, true, ActivityType::MESSAGE_FORWARDED));
                                }
                                
                                forwarded = false;
                            }
                            message = "";
                            frameCount = 1;
                            cout << "--------" << endl;
                        }
                        client.incoming_queue.pop();
                    };
                }
            }
        }
        else if (command == "PRINT_LOG") {
            string clientId, activityType, success;
            int logCount = 1;
            line >> clientId;
            char buffer[80];

            Client* client = findClient(clients, clientId);

            if (!client->log_entries.empty()) cout << "Client " << clientId << " Logs:" << endl;
            
            for (Log& log : client->log_entries) {
                cout << "--------------" << endl;
                if (log.activity_type == ActivityType::MESSAGE_RECEIVED) activityType = "Message Received";
                else if (log.activity_type == ActivityType::MESSAGE_FORWARDED) activityType = "Message Forwarded";
                else if (log.activity_type == ActivityType::MESSAGE_SENT) activityType = "Message Sent";
                else if (log.activity_type == ActivityType::MESSAGE_DROPPED) activityType = "Message Dropped";

                if (log.success_status) success = "Yes";
                else success = "No";

                cout << "Log Entry #" << logCount << ":" << endl;
                cout << "Activity: " << activityType << endl;
                cout << "Timestamp: " << log.timestamp << endl;
                cout << "Number of frames: " << log.number_of_frames << endl;
                cout << "Number of hops: " << log.number_of_hops << endl;
                cout << "Sender ID: " << log.sender_id << endl;
                cout << "Receiver ID: " << log.receiver_id << endl;
                cout << "Success: " << success << endl;
                if (log.activity_type == ActivityType::MESSAGE_RECEIVED || log.activity_type == ActivityType::MESSAGE_SENT) 
                cout << "Message: \"" << log.message_content <<  "\"" << endl;

                logCount++;

            }
        }
        else {
            cout << "Invalid command." << endl;
        }
    }
}

vector<Client> Network::read_clients(const string &filename) {
    vector<Client> clients;
    ifstream clientFile(filename);
    int clientCount = 0;
    string clientName, clientIp, clientMac;

    clientFile >> clientCount;

    while (clientFile >> clientName >> clientIp >> clientMac) {
        clients.push_back(Client(clientName, clientIp, clientMac));
    }
    return clients;
}

void Network::read_routing_tables(vector<Client> &clients, const string &filename) {
    ifstream routingFile(filename);

    int routingCount = 0;
    string routing, key, value;

    while (routingFile >> key) {
        if (key == "-") {
            routingCount++;
            continue;
        }

        routingFile >> value;
        clients[routingCount].routing_table[key] = value;
    }
}

vector<string> Network::read_commands(const string &filename) {
    vector<string> commands;
    ifstream commandFile(filename);
    string command;
    int commandCount = 0;

    commandFile >> commandCount;

    while (getline(commandFile, command)) commands.push_back(command);

    return commands;
}

Network::~Network() {
}