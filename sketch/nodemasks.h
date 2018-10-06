#ifndef SKT_NODEMASKS_H
#define SKT_NODEMASKS_H

#include <bitset>

namespace skt
{
  /** @anchor NodeMasks
  * @name node masks
  * openscenegraph node masks
  */
  ///@{
  typedef std::bitset<32> NodeMask; //!< typedef for node mask bit set.
  constexpr NodeMask WorkPlane(1 << 25); //!< Mask for the visible work plane.
  constexpr NodeMask WorkPlaneAxis(1 << 26); //!< both x and y axis lines.
  constexpr NodeMask WorkPlaneOrigin(1 << 27); //!< origin point.
  constexpr NodeMask Entity(1 << 28); //!< lines, arcs etc..
  constexpr NodeMask Constraint(1 << 29); //!< constraints.

  constexpr NodeMask SelectionPlane(1 << 30); //!< invisible selection plane.
  constexpr NodeMask ActiveSketch(1 << 31); //!< used to limit intersector.
  ///@}
  
  //! @brief The state of user interaction.
  enum class State
  {
    selection = 0 //!< no command running and user can select for future command.
    , point //!< the point command is running
    , line //!< the line command is running.
    , arc //!< the arc command is running.
    , drag //!< drag operation is commencing.
    , circle //!< the circle command is running.
  };
}

#endif // SKT_NODEMASKS_H
