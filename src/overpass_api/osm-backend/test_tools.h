#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__TEST_TOOLS_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__TEST_TOOLS_H


template< typename Obj >
class Compare_Vector
{
public:
  Compare_Vector(const std::string& title_) : title(title_)
  {
    std::cerr<<title<<" ... ";
  }

  Compare_Vector& operator()(const Obj& obj)
  {
    target.push_back(obj);
    return *this;
  }

  bool operator()(const std::vector< Obj >& candidate) const
  {
    bool all_ok = true;
    if (candidate.size() != target.size())
    {
      notify_failed(all_ok);
      std::cerr<<title<<": "<<target.size()<<" elements expected, "<<candidate.size()<<" elements found.\n";
    }
    for (decltype(target.size()) i = 0; i < target.size() && i < candidate.size(); ++i)
    {
      if (!(target[i] == candidate[i]))
      {
        notify_failed(all_ok);
        std::cerr<<title<<", element "<<i<<": found element different from expected one.\n";
      }
    }
    if (all_ok)
      std::cerr<<"ok.\n";
    return all_ok;
  }

private:
  static void notify_failed(bool& all_ok)
  {
    if (all_ok)
    {
      std::cerr<<"FAILED!\n";
      all_ok = false;
    }
  }

  std::string title;
  std::vector< Obj > target;
};


template< typename Key, typename Value >
class Compare_Map
{
public:
  Compare_Map(const std::string& title_) : title(title_)
  {
    std::cerr<<title<<" ... ";
  }

  Compare_Map& operator()(const Key& key, const Value& value)
  {
    target[key] = value;
    return *this;
  }

  bool operator()(const std::map< Key, Value >& candidate) const
  {
    bool all_ok = true;
    if (candidate.size() != target.size())
    {
      notify_failed(all_ok);
      std::cerr<<title<<": "<<target.size()<<" elements expected, "<<candidate.size()<<" elements found.\n";
    }
    auto i_target = target.begin();
    auto i_candidate = candidate.begin();
    while (i_target != target.end())
    {
      while (i_candidate != candidate.end() && i_candidate->first < i_target->first)
      {
        notify_failed(all_ok);
        std::cerr<<title<<": unexpected key skipped.\n";
        ++i_candidate;
      }
      if (i_candidate == candidate.end() || i_target->first < i_candidate->first)
      {
        notify_failed(all_ok);
        std::cerr<<title<<": expected key missing.\n";
      }
      if (i_candidate != candidate.end())
      {
        if (!(i_candidate->second == i_target->second))
        {
          notify_failed(all_ok);
          std::cerr<<title<<": values differ.\n";
        }
        ++i_candidate;
      }
      ++i_target;
    }
    while (i_candidate != candidate.end())
    {
      notify_failed(all_ok);
      std::cerr<<title<<": unexpected key skipped.\n";
      ++i_candidate;
    }
    if (all_ok)
      std::cerr<<"ok.\n";
    return all_ok;
  }

private:
  static void notify_failed(bool& all_ok)
  {
    if (all_ok)
    {
      std::cerr<<"FAILED!\n";
      all_ok = false;
    }
  }

  std::string title;
  std::map< Key, Value > target;
};


#endif
