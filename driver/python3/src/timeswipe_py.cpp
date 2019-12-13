#include <vector>
#include "timeswipe.hpp"
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "array_indexing_suite.h"
#include <iostream>

bool operator== (const Record &r1, const Record &r2)
{
    return r1.Sensors == r2.Sensors;
}

template <class T, class M> M get_member_type(M T:: *);
#define GET_TYPE_OF(mem) decltype(get_member_type(mem))

template <typename F>
auto GIL_WRAPPER(F&& f) {
    return [f=std::forward<F>(f)](auto&&... args) {
        auto gstate = PyGILState_Ensure();
        auto ret = f(std::forward<decltype(args)>(args)...);
        PyGILState_Release(gstate);
	return ret;
    };
}


BOOST_PYTHON_MODULE(timeswipe)
{
    using namespace boost::python;
    boost::python::class_<Record>("Record").add_property("sensors", &Record::Sensors 
            //,boost::python::return_value_policy<boost::python::return_by_value>()
            );
    boost::python::class_<std::vector<Record>>("RecordList")
        .def(boost::python::vector_indexing_suite<std::vector<Record>>());
    boost::python::class_<GET_TYPE_OF(&Record::Sensors)>("Sensor")
        .def(array_indexing_suite<GET_TYPE_OF(&Record::Sensors)>()) ;

    boost::python::class_<TimeSwipe, boost::noncopyable>("TimeSwipe")
        .def("SetBridge", &TimeSwipe::SetBridge,
                "Setup bridge number. It is mandatory to setup the bridge before Start")
        .def("SetSensorOffsets", &TimeSwipe::SetSensorOffsets,
               "Setup Sensor offsets. It is mandatory to setup offsets before Start" )
        .def("SetSensorGains", &TimeSwipe::SetSensorGains,
                "Setup Sensor gains. It is mandatory to setup gains before Start")
        .def("SetSensorTransmissions", &TimeSwipe::SetSensorTransmissions,
                "Setup Sensor transmissions. It is mandatory to setup transmissions before Start")
        .def("SetSecondary", &TimeSwipe::SetSecondary,
                "Setup secondary number")
        .def("Init", +[](TimeSwipe& self, boost::python::object bridge, boost::python::list offsets, boost::python::list gains, boost::python::list transmissions) {
            int br = boost::python::extract<int>(bridge);
            int ofs[4];
            int gns[4];
            double tr[4];
            for (int i = 0; i < 4; i++) {
                ofs[i] = boost::python::extract<int>(offsets[i]);
                gns[i] = boost::python::extract<int>(gains[i]);
                tr[i] = boost::python::extract<int>(transmissions[i]);
            }
            self.Init(br, ofs, gns, tr);
        },
            "This method is all-in-one replacement for SetBridge SetSensorOffsets SetSensorGains SetSensorTransmissions")
        .def("Start", +[](TimeSwipe& self, boost::python::object object) {
            self.Start(GIL_WRAPPER(object));
        },
            "Start reading Sensor loop. It is mandatory to setup SetBridge SetSensorOffsets SetSensorGains and SetSensorTransmissions before start. Only one instance of TimeSwipe can be running each moment of the time. After each sensor read complete cb called with vector of Record. Buffer is for 1 second data if cb works longer than 1 second, next data can be loosed and next callback called with non-zero errors")
        .def("SetSettings", &TimeSwipe::SetSettings,
            "Send SPI SetSettings request and receive the answer")
        .def("GetSettings", &TimeSwipe::GetSettings,
             "Send SPI GetSettings request and receive the answer")
        .def("onButton", +[](TimeSwipe& self, boost::python::object object) {
            self.onButton(GIL_WRAPPER(object));
        },
            "Register callback for button pressed/released. onButton must be called before called, otherwise register fails")
        .def("onError", +[](TimeSwipe& self, boost::python::object object) {
            self.onError(GIL_WRAPPER(object));
        },
            "onError must be called before Start called, otherwise register fails")
        .def("Stop", &TimeSwipe::Stop,
            "Stop reading Sensor loop")
    ;
}

