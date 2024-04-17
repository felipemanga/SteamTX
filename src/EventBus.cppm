module;

import <algorithm>;
import <iostream>;
import <memory>;
import <type_traits>;
import <vector>;

export module EventBus;

template<typename Sig>
struct signature;

template<typename R, typename ...Args>
struct signature<R(Args...)> {
    using type = std::tuple<Args...>;
};

template<typename C, typename R, typename ...Args>
struct signature<R(C::*)(Args...) const> {
    using type = std::tuple<Args...>;
};

template<typename C, typename R, typename ...Args>
struct signature<R(C::*)(Args...)> {
    using type = std::tuple<Args...>;
};

export class EventBus {
    struct Connection {
	EventBus* bus{};
	void (*func)(Connection&, void*);
    };

    template<typename Event>
    static std::vector<std::unique_ptr<Connection>>& getConnections() {
	static std::vector<std::unique_ptr<Connection>> connections;
	return connections;
    }

    std::vector<std::vector<std::unique_ptr<Connection>>*> buses;
    void* owner{};

public:
    EventBus() = default;

    template<typename Owner>
    EventBus(Owner* owner) : owner{owner} {}

    ~EventBus() {
	for (auto bus : buses) {
	    std::erase_if(*bus, [this](auto& c){return c->bus == this;});
	}
    }

    template<typename Callable>
    EventBus& operator >> (Callable&& cb) {
	using Args = typename signature<decltype(&Callable::operator())>::type;
	// using Args = typename signature<Callable>::type;
	using Arg0 = std::tuple_element_t<0, Args>;
	using Event = std::remove_cvref_t<Arg0>;
	struct CallableConnection : public Connection {
	    Callable cb;
	    CallableConnection(EventBus* bus, Callable&& cb) : cb{std::forward<Callable>(cb)} {
		this->bus = bus;
		this->func = +[](Connection& c, void* arg) {
		    auto& self = *static_cast<CallableConnection*>(&c);
		    auto& event = *reinterpret_cast<Event*>(arg);
		    self.cb(event);
		};
	    }
	};
	auto& connections = getConnections<Event>();
	connections.push_back(std::make_unique<CallableConnection>(this, std::forward<Callable>(cb)));
	buses.push_back(&connections);
	return *this;
    }

    template<typename Owner, typename Arg>
    EventBus& operator >> (void (Owner::*cb)(Arg)) {
	using Event = std::remove_cvref_t<Arg>;
	using Method = decltype(cb);
	struct CallableConnection : public Connection {
	    Method cb;
	    CallableConnection(EventBus* bus, Method cb) : cb{cb} {
		this->bus = bus;
		this->func = +[](Connection& c, void* arg) {
		    auto& self = *static_cast<CallableConnection*>(&c);
		    auto& event = *reinterpret_cast<Event*>(arg);
		    auto& owner = *reinterpret_cast<Owner*>(self.bus->owner);
		    (owner.*(self.cb))(event);
		};
	    }
	};
	auto& connections = getConnections<Event>();
	connections.push_back(std::make_unique<CallableConnection>(this, cb));
	buses.push_back(&connections);
	return *this;
    }

    template<typename Event>
    EventBus& operator << (Event&& event) {
	send(std::forward<Event>(event));
	return *this;
    }

    static inline bool verboseEvents{};

    template<typename Event>
    static void send(Event&& event) {
	using BaseEvent = std::remove_cvref_t<Event>;
	if (verboseEvents) {
	    std::cout << "Event: " << typeid(event).name() << std::endl;
	}
        auto& connections = getConnections<BaseEvent>();
	for (auto& connection : connections) {
	    connection->func(*connection, &event);
	}
    }
};
