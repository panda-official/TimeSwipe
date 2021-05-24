#include <algorithm>


class SensorsValues final {
public:
  using ValueType = float;

  class View final {
  public:
    ValueType& operator[](const std::size_t index) noexcept
    {
      assert(index < size_);
      return data_[index];
    }

    const ValueType& operator[](const std::size_t index) const noexcept
    {
      return static_cast<const ValueType&>(const_cast<View*>(this)->operator[](index));
    }

    explicit operator bool() const noexcept
    {
      return !!data_;
    }

  private:
    friend SensorsValues;

    ValueType* data_{};
    std::size_t size_{};

    View(ValueType* const data = {}, const std::size_t size = {}) noexcept
      : data_{data}
      , size_{size}
    {}
  };

  View operator[](std::size_t index) noexcept
  {
    if ( index = RealIndex(index); index != -1) {
      return View{data_.data(), ValuesPerSensor()};
    } else
      return View{};
  }

  const View operator[](const std::size_t index) const noexcept
  {
    return static_cast<const View>(const_cast<SensorsValues*>(this)->operator[](index));
  }

  std::size_t SensorCount() const noexcept
  {
    return count_if(cbegin(data_map_), cend(data_map_), std::equal_to{true});
  }

  std::size_t TotalSensorCount() const noexcept
  {
    return data_map_.size();
  }

  std::size_t ValuesPerSensor() const noexcept
  {
    return data_.size() / SensorCount();
  }

  SensorsValues(std::vector<ValueType> data, const std::size_t sensor_count)
    : data_{std::move(data)}
  {
    data_map_.resize(sensor_count, true);
  }

  SensorsValues(std::vector<ValueType> data, std::vector<bool> data_map)
    : data_{std::move(data)}
    , data_map_{std::move(data_map)}
  {}

private:
  std::vector<ValueType> data_;
  std::vector<bool> data_map_;

  std::size_t RealIndex(const std::size_t index) const noexcept
  {
    assert(index < data_map_.size());
    if (data_map_[index]) {
      const auto b = cbegin(data_map_);
      return count_if(b, b + index, std::equal_to{true});
    } else
      return -1;
  }
};
