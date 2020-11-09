#ifndef IDOCP_CONTACT_STATUS_HXX_
#define IDOCP_CONTACT_STATUS_HXX_

#include "idocp/robot/contact_status.hpp"

#include <cassert>

namespace idocp {

inline ContactStatus::ContactStatus(const int max_point_contacts)
  : is_contact_active_(max_point_contacts, false),
    dimf_(0),
    max_point_contacts_(max_point_contacts),
    num_active_contacts_(0),
    has_active_contacts_(false) {
}


inline ContactStatus::ContactStatus(const std::vector<bool>& is_contact_active)
  : is_contact_active_(is_contact_active),
    dimf_(0),
    max_point_contacts_(is_contact_active.size()),
    num_active_contacts_(0),
    has_active_contacts_(false) {
  setContactStatus(is_contact_active);
}


inline ContactStatus::ContactStatus() 
  : is_contact_active_(),
    dimf_(0),
    max_point_contacts_(0),
    num_active_contacts_(0),
    has_active_contacts_(false) {
}
 

inline ContactStatus::~ContactStatus() {
}


inline bool ContactStatus::operator==(const ContactStatus& other) const {
  assert(other.max_point_contacts() == max_point_contacts_);
  for (int i=0; i<max_point_contacts_; ++i) {
    if (other.isContactActive(i) != isContactActive(i)) {
      return false;
    }
  }
  return true;
}


inline bool ContactStatus::operator!=(const ContactStatus& other) const {
  return !(*this == other);
}


inline bool ContactStatus::isContactActive(const int contact_index) const {
  assert(!is_contact_active_.empty());
  assert(contact_index >= 0);
  assert(contact_index < is_contact_active_.size());
  return is_contact_active_[contact_index];
}


inline const std::vector<bool>& ContactStatus::isContactActive() const {
  return is_contact_active_;
}


inline bool ContactStatus::hasActiveContacts() const {
  return has_active_contacts_;
}


inline int ContactStatus::dimf() const {
  return dimf_;
}


inline int ContactStatus::num_active_contacts() const {
  return num_active_contacts_;
}


inline int ContactStatus::max_point_contacts() const {
  return max_point_contacts_;
}


inline void ContactStatus::setContactStatus(
    const std::vector<bool>& is_contact_active) {
  assert(is_contact_active.size() == max_point_contacts_);
  is_contact_active_ = is_contact_active;
  dimf_ = 0;
  num_active_contacts_ = 0;
  for (const auto e : is_contact_active) {
    if (e) {
      dimf_ += 3;
      ++num_active_contacts_;
    }
  }
  set_has_active_contacts();
}


inline void ContactStatus::activateContact(const int contact_index) {
  assert(contact_index >= 0);
  assert(contact_index < max_point_contacts_);
  if (!is_contact_active_[contact_index]) {
    is_contact_active_[contact_index] = true;
    dimf_ += 3;
    ++num_active_contacts_;
  }
  set_has_active_contacts();
}


inline void ContactStatus::deactivateContact(const int contact_index) {
  assert(contact_index >= 0);
  assert(contact_index < max_point_contacts_);
  if (is_contact_active_[contact_index]) {
    is_contact_active_[contact_index] = false;
    dimf_ -= 3;
    --num_active_contacts_;
  }
  set_has_active_contacts();
}


inline void ContactStatus::activateContacts(
    const std::vector<int>& contact_indices) {
  assert(contact_indices.size() <= max_point_contacts_);
  for (const int index : contact_indices) {
    assert(index >= 0);
    assert(index < max_point_contacts_);
    if (!is_contact_active_[index]) {
      is_contact_active_[index] = true;
      dimf_ += 3;
      ++num_active_contacts_;
    }
  }
  set_has_active_contacts();
}
 

inline void ContactStatus::deactivateContacts(
    const std::vector<int>& contact_indices) {
  assert(contact_indices.size() <= max_point_contacts_);
  for (const int index : contact_indices) {
    assert(index >= 0);
    assert(index < max_point_contacts_);
    if (is_contact_active_[index]) {
      is_contact_active_[index] = false;
      dimf_ -= 3;
      --num_active_contacts_;
    }
  }
  set_has_active_contacts();
}


inline void ContactStatus::set_has_active_contacts() {
  if (num_active_contacts_ > 0) {
    has_active_contacts_ = true;
  }
  else {
    has_active_contacts_ = false;
  }
}

} // namespace idocp

#endif // IDOCP_CONTACT_STATUS_HXX_ 