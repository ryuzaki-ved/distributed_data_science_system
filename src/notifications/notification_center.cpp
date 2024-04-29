#include "../../include/notifications/notification_center.h"
#include <iostream>
#include <algorithm>

namespace dds {
namespace notifications {

void NotificationCenter::send_notification(const Notification& notif) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    notifications_.push_back(notif);
    user_inbox_[notif.recipient].push_back(notif);
    unread_counts_[notif.recipient]++;
    for (const auto& listener : listeners_) {
        listener(notif);
    }
    std::cout << "🔔 Notification sent to " << notif.recipient << ": " << notif.title << std::endl;
}

void NotificationCenter::broadcast(const Notification& notif) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    for (auto& pair : user_inbox_) {
        Notification copy = notif;
        copy.recipient = pair.first;
        pair.second.push_back(copy);
        unread_counts_[pair.first]++;
    }
    notifications_.push_back(notif);
    for (const auto& listener : listeners_) {
        listener(notif);
    }
    std::cout << "📢 Broadcast notification: " << notif.title << std::endl;
}

void NotificationCenter::mark_as_read(const std::string& notif_id, const std::string& user) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    auto& inbox = user_inbox_[user];
    for (auto& notif : inbox) {
        if (notif.id == notif_id && !notif.read) {
            notif.read = true;
            unread_counts_[user]--;
        }
    }
}

std::vector<Notification> NotificationCenter::get_user_notifications(const std::string& user, bool unread_only) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    std::vector<Notification> result;
    for (const auto& notif : user_inbox_[user]) {
        if (!unread_only || !notif.read) {
            result.push_back(notif);
        }
    }
    return result;
}

int NotificationCenter::get_unread_count(const std::string& user) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    return unread_counts_[user];
}

void NotificationCenter::add_listener(std::function<void(const Notification&)> listener) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    listeners_.push_back(listener);
}

void NotificationCenter::clear_user_notifications(const std::string& user) {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    user_inbox_[user].clear();
    unread_counts_[user] = 0;
}

void NotificationCenter::print_summary() const {
    std::lock_guard<std::mutex> lock(notif_mutex_);
    std::cout << "\n🔔 Notification Center Summary" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Total notifications: " << notifications_.size() << std::endl;
    for (const auto& pair : user_inbox_) {
        std::cout << "User: " << pair.first << ", Inbox: " << pair.second.size() 
                  << ", Unread: " << unread_counts_.at(pair.first) << std::endl;
    }
}

} // namespace notifications
} // namespace dds
