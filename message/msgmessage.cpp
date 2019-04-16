/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <boost/variant/get.hpp>

#include <osg/Node>

#include <message/variant.h>
#include <message/message.h>

using namespace msg;

Message::Message() : mask(0){}

Message::Message(const Mask &maskIn) : mask(maskIn){}

Message::Message(const Mask &maskIn, const Stow &sIn)
: mask(maskIn)
, stow(std::make_shared<Stow>(sIn)){}

Message::Message(const Mask &mIn, const prj::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

Message::Message(const Mask &mIn, const app::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

Message::Message(const Mask &mIn, const slc::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

Message::Message(const Mask &mIn, const vwr::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

Message::Message(const Mask &mIn, const ftr::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

Message::Message(const Mask &mIn, const lod::Message &m)
: mask(mIn)
, stow(std::make_shared<Stow>(m)){}

const prj::Message& Message::getPRJ() const
{
  assert(stow->variant.type() == typeid(prj::Message));
  return boost::get<prj::Message>(stow->variant);
}

const app::Message& Message::getAPP() const
{
  assert(stow->variant.type() == typeid(app::Message));
  return boost::get<app::Message>(stow->variant);
}

const slc::Message& Message::getSLC() const
{
  assert(stow->variant.type() == typeid(slc::Message));
  return boost::get<slc::Message>(stow->variant);
}

const vwr::Message& Message::getVWR() const
{
  assert(stow->variant.type() == typeid(vwr::Message));
  return boost::get<vwr::Message>(stow->variant);
}

const ftr::Message& Message::getFTR() const
{
  assert(stow->variant.type() == typeid(ftr::Message));
  return boost::get<ftr::Message>(stow->variant);
}

const lod::Message& Message::getLOD() const
{
  assert(stow->variant.type() == typeid(lod::Message));
  return boost::get<lod::Message>(stow->variant);
}

msg::Message msg::buildGitMessage(const std::string &messageIn)
{
  prj::Message pMessage;
  pMessage.gitMessage = messageIn;
  return msg::Message
  (
    msg::Request | msg::Git | msg::Text
    , pMessage
  );
}

msg::Message msg::buildStatusMessage(const std::string &messageIn)
{
  return msg::Message
  (
    msg::Request | msg::Status | msg::Text
    , vwr::Message(messageIn)
  );
}

msg::Message msg::buildStatusMessage(const std::string &messageIn, float timeIn)
{
  return msg::Message
  (
    msg::Request | msg::Status | msg::Text
    , vwr::Message(messageIn, timeIn)
  );
}

msg::Message msg::buildSelectionMask(slc::Mask maskIn)
{
  return msg::Message
  (
    msg::Request | msg::Selection | msg::SetMask
    , slc::Message(maskIn)
  );
}

msg::Message msg::buildShowThreeD(const boost::uuids::uuid &idIn)
{
  return msg::Message
  (
    msg::Request | msg::View | msg::Show | msg::ThreeD
    , vwr::Message(idIn)
  );
}

msg::Message msg::buildHideThreeD(const boost::uuids::uuid &idIn)
{
  return msg::Message
  (
    msg::Request | msg::View | msg::Hide | msg::ThreeD
    , vwr::Message(idIn)
  );
}

msg::Message msg::buildShowOverlay(const boost::uuids::uuid &idIn)
{
  return msg::Message
  (
    msg::Request | msg::View | msg::Show | msg::Overlay
    , vwr::Message(idIn)
  );
}

msg::Message msg::buildHideOverlay(const boost::uuids::uuid &idIn)
{
  return msg::Message
  (
    msg::Request | msg::View | msg::Hide | msg::Overlay
    , vwr::Message(idIn)
  );
}
