#include <vector>
#include "timeswipe.hpp"
#include <boost/python.hpp>
#include <iostream>

template<class T>
struct VecToList
{
    static PyObject* convert(const std::vector<T>& vec)
    {
        boost::python::list* l = new boost::python::list();
        for(size_t i = 0; i < vec.size(); i++) {
            l->append(vec[i]);
        }

        return l->ptr();
    }
};

struct RecordToList
{
    static PyObject* convert(const Record& rec)
    {
        boost::python::list* l = new boost::python::list();
        for(size_t i = 0; i < rec.Sensors.size(); i++) {
            l->append(rec.Sensors[i]);
        }

        return l->ptr();
    }
};

BOOST_PYTHON_MODULE(timeswipe)
{
    boost::python::to_python_converter<Record, RecordToList>();

    boost::python::to_python_converter<std::vector<Record, std::allocator<Record> >, VecToList<Record> >();
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
            self.Start(object);
        },
            "Start reading Sensor loop. It is mandatory to setup SetBridge SetSensorOffsets SetSensorGains and SetSensorTransmissions before start. Only one instance of TimeSwipe can be running each moment of the time. After each sensor read complete cb called with vector of Record. Buffer is for 1 second data if cb works longer than 1 second, next data can be loosed and next callback called with non-zero errors")
        .def("SetSettings", &TimeSwipe::SetSettings,
            "Send SPI SetSettings request and receive the answer")
        .def("GetSettings", &TimeSwipe::GetSettings,
             "Send SPI GetSettings request and receive the answer")
        .def("onButton", +[](TimeSwipe& self, boost::python::object object) {
            self.onButton(object);
        },
            "Register callback for button pressed/released. onButton must be called before called, otherwise register fails")
        .def("onError", +[](TimeSwipe& self, boost::python::object object) {
            self.onError(object);
        },
            "onError must be called before Start called, otherwise register fails")
        .def("Stop", &TimeSwipe::Stop,
            "Stop reading Sensor loop")
    ;
}

