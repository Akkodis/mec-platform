#pragma once
#include <queue>
#include <atomic>
#include <sstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <any>

#include <proton/connection.hpp>
#include <proton/reconnect_options.hpp>
#include <proton/connection_options.hpp>
#include <proton/container.hpp>
#include <proton/message.hpp>
#include <proton/message_id.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/tracker.hpp>
#include <proton/types.hpp>
#include <proton/receiver_options.hpp>
#include <proton/source_options.hpp>
#include <proton/delivery.hpp>
#include <proton/map.hpp>
#include <proton/ssl.hpp>
#include <proton/listen_handler.hpp>
#include <proton/listener.hpp>
#include <proton/work_queue.hpp>

//#include "SimpleAmqpClient/SimpleAmqpClient.h"


typedef void (*message_callback)(proton::delivery d, proton::message msg);

namespace AMQP_1_0{

    class CustomProperties{
        public:
        //guida JMS https://timjansen.github.io/jarfiller/guide/jms/selectors.xhtml#
        std::pair<std::string, std::any> prop; //KEY, VALUE
        std::string operation;                    // =, LIKE

        CustomProperties(std::string key, int value, std::string op) {
            prop = std::make_pair(key,std::make_any<int>(value));
            operation = op;
        }
        CustomProperties(std::string key, int64_t value, std::string op) {
            prop = std::make_pair(key,std::make_any<int64_t>(value));
            operation = op;
        }
        CustomProperties(std::string key, double value, std::string op) {
            prop = std::make_pair(key,std::make_any<double>(value));
            operation = op;
        }
        CustomProperties(std::string key, std::string value, std::string op) {
            prop = std::make_pair(key,std::make_any<std::string>(value));
            operation = op;
        }
    };
    
    /*** 
     * Subscriber constructor
     * ip_address broker ip + port,
     * connection info: user,
     * password connection info: password,
     * topic connection topic
     */
    class Subscriber : public proton::messaging_handler{
        std::string url;
        std::string user;
        std::string password;
        std::string topic;
        std::string selector_string;
        proton::receiver receiver;
        // std::vector<CustomProperties> filters;
        message_callback fun_callback;

    public:
        
        Subscriber(const std::string &s, const std::string &u, const std::string &p, const std::string t) :
            url(s), user(u), password(p), topic(t) { fun_callback = NULL; selector_string="";}

        Subscriber();
        void set_subscriber(const std::string &s, const std::string &u, const std::string &p, const std::string t);
        
        void on_container_start(proton::container &c);
        void on_message(proton::delivery &d, proton::message &msg);
        // void set_filters(const std::vector<CustomProperties> &filters);
        void set_selector_string(const std::string& selector_string);
        void set_callback(message_callback fun_callback);
        void close_subscriber();
    private:
        void on_error(const proton::error_condition& e) override {
                    std::cout << e.what() << std::endl;
        }
    };

    /*** 
     *  PUBLISHER CLASS
     */
   class Publisher : private proton::messaging_handler {
        // Only used in proton handler thread
        proton::sender sender_;
        // Shared by proton and user threads, protected by lock_
        std::mutex lock_;
        proton::work_queue *work_queue_;
        std::condition_variable sender_ready_;
        int queued_;                       // Queued messages waiting to be sent
        int credit_;   

        public:
            Publisher(proton::container& cont, const std::string& url, const std::string& user, const std::string& password, const std::string& destination_topic);
            Publisher();
            void set_publisher(proton::container& cont, const std::string& url, const std::string& user, const std::string& password, const std::string& destination_topic);
            // Thread safe
            void send(const proton::message& m);
            // Thread safe
            void close();

        private:
            proton::work_queue* work_queue();

            // == messaging_handler overrides, only called in proton handler thread
            void on_sender_open(proton::sender& s);
            void on_sendable(proton::sender& s);
            void do_send(const proton::message& m);
            void on_error(const proton::error_condition& e) override {
                std::cout << e.what() << std::endl;
            }
    };

    class Utilities {
    public:
        static std::string createSelectorString(const std::vector<AMQP_1_0::CustomProperties> &props);
        static std::string addSelector(std::string selector_string, std::string property_name, std::string operation, std::string value, std::string logic_value);
        static std::string addSelector(std::string selector_string, std::string property_name, std::string operation, int value, std::string logic_value);
        static std::string addSelector(std::string selector_string, std::string property_name, std::string operation, int64_t value, std::string logic_value);
        static std::string addSelector(std::string selector_string, std::string property_name, std::string operation, double value, std::string logic_value);
        static proton::source::filter_map getFilter(const std::string& selector_string);
    private:
        static std::string addSelector(std::string selector_string, std::string property_name, std::string operation, std::any value, std::string logic_value);
    };
}


//namespace AMQP_0_9
//{

//    class Publisher
//    {
//    private:
//        std::string address;
//        int port;
//        std::string user;
//        std::string password;
//        std::string exchange_name;
//        std::string queue_name;
//        AmqpClient::Channel::ptr_t channel;
//        std::string routing_key;
//        std::string cert_;
//        std::string pkey_;

//    public:
//        Publisher() {}
//        Publisher(const std::string &host, int port, const std::string &user, const std::string &password)
//            : address(host), port(port), user(user), password(password)
//        {
//            AmqpClient::Channel::OpenOpts opts;
//            opts.host = host;
//            opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(user, password);
//            opts.port = port;
//            opts.vhost = "/";
//            channel = AmqpClient::Channel::Open(opts);
//        }

//        Publisher(const std::string &host, int port, const std::string &user, const std::string &password, const std::string &cert_path, const std::string &pkey_path)
//            : address(host), port(port), user(user), password(password), cert_(cert_path)
//        {

//            AmqpClient::Channel::OpenOpts::TLSParams tls_params;
//            tls_params.client_cert_path = cert_path;
//            tls_params.client_key_path = pkey_path;
//            AmqpClient::Channel::OpenOpts opts;
//            opts.host = host;
//            opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(user, password);
//            opts.port = port;
//            opts.vhost = "/";
//            opts.tls_params = tls_params;
//            channel = AmqpClient::Channel::Open(opts);
//        }
//        void setExchangeName(const std::string &exchange_name);
//        void setQueueName(const std::string &queue_name);
//        void setRoutingKey(const std::string &routing_key);

//        void declareExchangeOrQueue(const std::string exchange_type_topic, bool passive, bool durable, bool auto_delete, const bool exlusive);
//        // void declareQueue(const bool passive,const bool durable, const bool exclusive, const bool auto_delete);
//        void bindQueue();

//        // expiration is in ms
//        void publish(const std::string &payload, const int expiration);

//        void start_test();
//    };

//    class Subscriber
//    {
//    private:
//        const std::string address;
//        int port;
//        const std::string user;
//        const std::string password;
//        std::string exchange_name;
//        std::string queue_name;
//        AmqpClient::Channel::ptr_t channel;
//        std::string routing_key;
//        std::string consumer_tag;
//        std::string cert_;
//        std::string pkey_;

//    public:
//        Subscriber() {}
//        Subscriber(const std::string &host, int port, const std::string &user, const std::string &password)
//            : address(host), port(port), user(user), password(password)
//        {
//            AmqpClient::Channel::OpenOpts opts;
//            opts.host = host;
//            opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(user, password);
//            opts.port = port;
//            opts.vhost = "/";
//            channel = AmqpClient::Channel::Open(opts);
//        }

//        Subscriber(
//            const std::string &host,
//            int port, const std::string &user,
//            const std::string &password, const std::string &cert_path, const std::string &pkey_path)
//            : address(host), port(port), user(user), password(password), cert_(cert_path), pkey_(pkey_path)
//        {

//            AmqpClient::Channel::OpenOpts::TLSParams tls_params;
//            tls_params.client_cert_path = cert_;
//            tls_params.client_key_path = pkey_;
//            tls_params.verify_peer = true;
//            tls_params.verify_hostname = false;
//            AmqpClient::Channel::OpenOpts opts;
//            opts.host = host;
//            opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(user, password);
//            opts.port = port;
//            opts.vhost = "/";
//            opts.tls_params = tls_params;
//            channel = AmqpClient::Channel::Open(opts);
//            std::cout << "Creating TLS channel " << std::endl;
//        }

//        void setExchangeName(const std::string &exchange_name);
//        void setQueueName(const std::string &queue_name);
//        void setRoutingKey(const std::string &routing_key);
//        void declareExchangeOrQueue(const std::string exchange_type_topic, bool passive, bool durable, bool auto_delete, const bool exlusive);
//        //void bindQueue(const std::string &exchange_name, const std::string queue_name, const std::string &routing_key);
//        void bindQueue();
//        void setConsumerTag(const std::string tag, const bool no_local, const bool no_ack, const bool exclusive, const boost::uint16_t message_prefetch_count);
//        std::pair<std::string, std::string> consume();
//        std::string getRoutingKey();
//        void cancelSubscription();

//        void start_test();
//    };
//}
