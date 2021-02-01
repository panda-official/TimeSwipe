#include <string>

class PidFile {
    std::string fname;
    int fd = -1;
    bool locked = false;
    void unlock();
public:
    /**
      * \brief Create PidFile
      * @param name - unique application name
      */
    PidFile(const std::string& name);
    ~PidFile();

    /**
      * \brief lock name
      * if first call was successful then second and after call will be successful too
      * @param error - error description when lock is failed
      *
      * @return false if pidloc failed, and error has detailed error description
      */
    bool Lock(std::string& error);
};
