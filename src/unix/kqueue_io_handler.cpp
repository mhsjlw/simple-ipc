#include "kqueue_io_handler.h"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

using namespace simpleipc;

kqueue_io_handler::kqueue_io_handler() {
    kq = kqueue();

    thread = std::thread(std::bind(&kqueue_io_handler::run, this));
}

kqueue_io_handler::~kqueue_io_handler() {
    cbm.lock();
    running = false;
    cbm.unlock();
    thread.join();
    close(kq);
}

void kqueue_io_handler::add_socket(int fd, fd_callback data_cb, fd_callback close_cb) {
    struct kevent k_set;
    EV_SET(&k_set, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &k_set, 1, NULL, 0, NULL);

    cbm.lock();
    cbs[fd] = {data_cb, close_cb};
    cbm.unlock();
}

void kqueue_io_handler::remove_socket(int fd) {
    struct kevent k_set;
    EV_SET(&k_set, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kq, &k_set, 1, NULL, 0, NULL);

    cbm.lock();
    cbs.erase(fd);
    cbm.unlock();
}

void kqueue_io_handler::run() {
    struct kevent ev_set;
    struct kevent ev_list[64];
    int n;

    while(true) {
        cbm.lock();
        n = kevent(kq, NULL, 0, ev_list, 64, NULL);
        for(int i = 0; i < n; i++) {
            auto cb = cbs.find(ev_list[i].ident);
            if (cb == cbs.end())
                continue;

            if (ev_list[i].flags & EV_EOF) {
                if (cb->second.close_cb)
                    cb->second.close_cb(ev_list[i].ident);
            } else if (ev_list[i].filter & EVFILT_READ) {
                if (cb->second.data_cb)
                    cb->second.data_cb(ev_list[i].ident);
            }
        }

        if(!running)
            break;
        cbm.unlock();
    }
}

#ifdef __APPLE__
io_handler& io_handler::get_instance() {
    static kqueue_io_handler instance;
    return instance;
}
#endif