



#include "feature/ftrbase.h"
#include "feature/ftrupdatepayload.h"


using namespace ftr;

std::vector<const Base*> UpdatePayload::getFeatures(const std::string &tag) const
{
  return getFeatures(updateMap, tag);
}

std::vector<const Base*> UpdatePayload::getFeatures(const UpdateMap &updateMapIn, const std::string &tag)
{
  std::vector<const Base*> out;
  if (tag.empty())
  {
    for (const auto &p : updateMapIn)
      out.push_back(p.second);
  }
  else
  {
    for (auto its = updateMapIn.equal_range(tag); its.first != its.second; ++its.first)
      out.push_back(its.first->second);
  }
  
  std::sort(out.begin(), out.end());
  auto last = std::unique(out.begin(), out.end());
  out.erase(last, out.end());
  
  return out;
}
