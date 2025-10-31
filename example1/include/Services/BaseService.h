#ifndef BASE_SERVICE_H
#define BASE_SERVICE_H

template<typename ContextType>
class BaseService {
public:
    virtual ~BaseService() {}
    virtual void init() = 0;
    virtual void update(ContextType& context) = 0;
    virtual unsigned long getUpdateInterval() = 0; // Возвращает время задержки для сервиса
};

#endif // BASE_SERVICE_H