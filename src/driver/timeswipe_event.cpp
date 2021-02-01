#include "timeswipe.hpp"

#include <variant>

class TimeSwipeEventImpl {
    std::variant<
        TimeSwipeEvent::Button,
        TimeSwipeEvent::Gain,
        TimeSwipeEvent::SetSecondary,
        TimeSwipeEvent::Bridge,
        TimeSwipeEvent::Record,
        TimeSwipeEvent::Offset,
        TimeSwipeEvent::Mode
        > events;
    friend class TimeSwipeEvent;
};

TimeSwipeEvent::TimeSwipeEvent() {
    _impl = std::make_shared<TimeSwipeEventImpl>();
}

TimeSwipeEvent::~TimeSwipeEvent() {
}

TimeSwipeEvent::Button::Button(bool _pressed, unsigned _count)
    : _pressed(_pressed)
    , _count(_count)
{
}
bool TimeSwipeEvent::Button::pressed() const {
    return _pressed;
}
unsigned TimeSwipeEvent::Button::count() const {
    return _count;
}

TimeSwipeEvent::Gain::Gain(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::Gain::value() const {
    return _value;
}

TimeSwipeEvent::SetSecondary::SetSecondary(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::SetSecondary::value() const {
    return _value;
}

TimeSwipeEvent::Bridge::Bridge(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::Bridge::value() const {
    return _value;
}

TimeSwipeEvent::Record::Record(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::Record::value() const {
    return _value;
}

TimeSwipeEvent::Offset::Offset(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::Offset::value() const {
    return _value;
}

TimeSwipeEvent::Mode::Mode(int _value)
    : _value(_value)
{
}
int TimeSwipeEvent::Mode::value() const {
    return _value;
}

template <class EVENT>
bool TimeSwipeEvent::is() const {
    return std::holds_alternative<EVENT>(_impl->events);
}
template bool TimeSwipeEvent::is<TimeSwipeEvent::Button>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::Gain>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::SetSecondary>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::Bridge>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::Record>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::Offset>() const;
template bool TimeSwipeEvent::is<TimeSwipeEvent::Mode>() const;

template <class EVENT>
const EVENT& TimeSwipeEvent::get() const {
    return std::get<EVENT>(_impl->events);
}

template const TimeSwipeEvent::Button& TimeSwipeEvent::get<TimeSwipeEvent::Button>() const;
template const TimeSwipeEvent::Gain& TimeSwipeEvent::get<TimeSwipeEvent::Gain>() const;
template const TimeSwipeEvent::SetSecondary& TimeSwipeEvent::get<TimeSwipeEvent::SetSecondary>() const;
template const TimeSwipeEvent::Bridge& TimeSwipeEvent::get<TimeSwipeEvent::Bridge>() const;
template const TimeSwipeEvent::Record& TimeSwipeEvent::get<TimeSwipeEvent::Record>() const;
template const TimeSwipeEvent::Offset& TimeSwipeEvent::get<TimeSwipeEvent::Offset>() const;
template const TimeSwipeEvent::Mode& TimeSwipeEvent::get<TimeSwipeEvent::Mode>() const;

template <class EVENT>
TimeSwipeEvent::TimeSwipeEvent(EVENT&& ev)
    : TimeSwipeEvent()
{
    _impl->events = std::move(ev);
}
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Button&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Gain&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::SetSecondary&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Bridge&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Record&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Offset&& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Mode&& ev);

template <class EVENT>
TimeSwipeEvent::TimeSwipeEvent(const EVENT& ev)
    : TimeSwipeEvent()
{
    _impl->events = ev;
}
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Button& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Gain& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::SetSecondary& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Bridge& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Record& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Offset& ev);
template TimeSwipeEvent::TimeSwipeEvent(TimeSwipeEvent::Mode& ev);
