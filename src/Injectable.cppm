module;

import <memory>;

export module Injectable;

export template<typename Base> class Injectable : public std::enable_shared_from_this<Injectable<Base>> {
public:
    virtual ~Injectable() = default;

    static inline std::shared_ptr<Base> (*create)() = []{return std::shared_ptr<Base>{};};

    template<typename Derived>
    class Register {
    public:
	Register(){
	    create = []{return std::static_pointer_cast<Base>(std::make_shared<Derived>());};
	}
    };
};
