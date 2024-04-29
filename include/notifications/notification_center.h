#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <chrono>

namespace dds {
namespace notifications {

// Notification types
enum class NotificationType {
    INFO,
    WARNING,
    ERROR,
    SUCCESS,
    ALERT,
    SYSTEM,
    USER
};

// Notification structure
struct Notification {
    std::string id;
    NotificationType type;
    std::string title;
    std::string message;
    std::string recipient;
    std::chrono::system_clock::time_point timestamp;
    bool read;
    std::string channel;
    std::map<std::string, std::string> metadata;
};

// Notification center
class NotificationCenter {
private:
    std::vector<Notification> notifications_;
    std::mutex notif_mutex_;
    std::map<std::string, std::vector<Notification>> user_inbox_;
    std::map<std::string, int> unread_counts_;
    std::vector<std::function<void(const Notification&)>> listeners_;

public:
    // Send notification to a user
    void send_notification(const Notification& notif);
    // Broadcast to all users
    void broadcast(const Notification& notif);
    // Mark as read
    void mark_as_read(const std::string& notif_id, const std::string& user);
    // Get notifications for a user
    std::vector<Notification> get_user_notifications(const std::string& user, bool unread_only = false);
    // Get unread count
    int get_unread_count(const std::string& user);
    // Add listener for notifications
    void add_listener(std::function<void(const Notification&)> listener);
    // Clear notifications for a user
    void clear_user_notifications(const std::string& user);
    // Print summary
    void print_summary() const;
};

} // namespace notifications
} // namespace dds
