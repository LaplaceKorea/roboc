#ifndef IDOCP_CONTACT_STATUS_HPP_
#define IDOCP_CONTACT_STATUS_HPP_

#include <vector>


namespace idocp {

  ///
  /// @class ContactStatus
  /// @brief Contact status.
  ///
class ContactStatus {
public:
  ///
  /// @brief Constructor. 
  /// @param[in] max_point_contacts Maximum number of the point contacts. 
  ///
  ContactStatus(const int max_point_contacts);

  ///
  /// @brief Constructor. 
  /// @param[in] is_contact_active Vector containing bool representing that each 
  /// point contact is active or not. 
  ///
  ContactStatus(const std::vector<bool>& is_contact_active);

  ///
  /// @brief Default constructor. 
  ///
  ContactStatus();

  ///
  /// @brief Destructor. 
  ///
  ~ContactStatus();

  ///
  /// @brief Default copy constructor. 
  ///
  ContactStatus(const ContactStatus&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  ContactStatus& operator=(const ContactStatus&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  ContactStatus(ContactStatus&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  ContactStatus& operator=(ContactStatus&&) noexcept = default;

  ///
  /// @brief Define comparison operator. 
  ///
  bool operator==(const ContactStatus& other) const;

  ///
  /// @brief Define comparison operator. 
  ///
  bool operator!=(const ContactStatus& other) const;

  ///
  /// @brief Return true if a contact is active and false if not.
  /// @param[in] contact_index Index of a contact of interedted. 
  /// @return true if a contact is active and false if not. 
  ///
  bool isContactActive(const int contact_index) const;

  ///
  /// @brief Return contact status.
  /// @return Const reference to the contact status. 
  ///
  const std::vector<bool>& isContactActive() const;

  ///
  /// @brief Return true if there are active contacts and false if not.
  /// @return true if there are active contacts and false if not. 
  ///
  bool hasActiveContacts() const;

  ///
  /// @brief Return the dimension of the active contacts.
  /// @return Dimension of the active contacts. 
  ///
  int dimf() const;

  ///
  /// @brief Return the number of the active contacts.
  /// @return The number of the active contacts. 
  ///
  int num_active_contacts() const;

  ///
  /// @brief Return the maximum number of the contacts.
  /// @return The maximum number of the contacts. 
  ///
  int max_point_contacts() const;

  ///
  /// @brief Set from other contact status that has the same size.
  /// @param[in] other Other contact status. 
  ///
  void set(const ContactStatus& other);

  ///
  /// @brief Set the contact status.
  /// @param[in] is_contact_active Contact status. Size must be 
  /// ContactStatus::max_point_contacts();
  ///
  void setContactStatus(const std::vector<bool>& is_contact_active);

  ///
  /// @brief Activate a contact.
  /// @param[in] contact_index Index of the contact that is activated.
  ///
  void activateContact(const int contact_index);

  ///
  /// @brief Deactivate a contact.
  /// @param[in] contact_index Index of the contact that is deactivated.
  ///
  void deactivateContact(const int contact_index);

  ///
  /// @brief Activate contacts.
  /// @param[in] contact_indices Indices of the contacts that are activated.
  ///
  void activateContacts(const std::vector<int>& contact_indices);

  ///
  /// @brief Deactivate contacts.
  /// @param[in] contact_indices Indices of the contacts that are deactivated.
  ///
  void deactivateContacts(const std::vector<int>& contact_indices);

  ///
  /// @brief Fill contact status randomly.
  ///
  void setRandom();

private:
  std::vector<bool> is_contact_active_;
  int dimf_, max_point_contacts_, num_active_contacts_;
  bool has_active_contacts_;

  void set_has_active_contacts();

};

} // namespace idocp

#include "idocp/robot/contact_status.hxx"

#endif // IDOCP_CONTACT_STATUS_HPP_ 