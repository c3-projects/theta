#include "c3/theta/channel/bluetooth.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "common.UNIX.hxx"

#include <iostream>
#include <c3/nu/data/encoders/hex.hpp>

namespace c3::theta {
  template<>
  struct _sockaddr_t<bluetooth::rfcomm_ep_t> { using type = sockaddr_rc; };
  template<>
  struct _baseep_t<sockaddr_rc> { using type = bluetooth::rfcomm_ep_t; };

  template<>
  inline ::sockaddr_rc ep2sockaddr(const bluetooth::rfcomm_ep_t& ep) {
    ::sockaddr_rc ret;

    memset(&ret, 0, sizeof(ret));

    ret.rc_family = AF_BLUETOOTH;
    ret.rc_channel = ep.port;
    std::copy(ep.addr.rbegin(), ep.addr.rend(), ret.rc_bdaddr.b);

    return ret;
  }
  template<>
  inline bluetooth::rfcomm_ep_t sockaddr2ep(const ::sockaddr_rc& ep_struct) {
    bluetooth::rfcomm_ep_t ep;
    std::copy(ep_struct.rc_bdaddr.b,
              ep_struct.rc_bdaddr.b + ep.addr.size(),
              ep.addr.rbegin());
    ep.port = ep_struct.rc_channel;
    return ep;
  }

  template<>
  constexpr int af<bluetooth::address>() { return AF_BLUETOOTH; };
}

namespace c3::theta {
  template<>
  struct _sockaddr_t<bluetooth::l2cap_ep_t> { using type = sockaddr_l2; };
  template<>
  struct _baseep_t<sockaddr_l2> { using type = bluetooth::l2cap_ep_t; };

  //TODO: maybe add LE BT?
  template<>
  inline ::sockaddr_l2 ep2sockaddr(const bluetooth::l2cap_ep_t& ep) {
    ::sockaddr_l2 ret;

    memset(&ret, 0, sizeof(ret));

    ret.l2_family = AF_BLUETOOTH;
    ret.l2_psm = htole16(ep.port.psm);
    // Connectionless channel id
    ret.l2_cid = htole16(ep.port.cid);
    std::copy(ep.addr.begin(), ep.addr.end(), ret.l2_bdaddr.b);

    return ret;
  }
  template<>
  inline bluetooth::l2cap_ep_t sockaddr2ep(const ::sockaddr_l2& ep_struct) {
    bluetooth::l2cap_ep_t ep;
    std::copy(ep_struct.l2_bdaddr.b,
              ep_struct.l2_bdaddr.b + ep.addr.size(),
              ep.addr.begin());
    ep.port.psm = le16toh(ep_struct.l2_psm);
    ep.port.cid = le16toh(ep_struct.l2_cid);
    return ep;
  }
}

namespace c3::theta::bluetooth {
  class _rfcomm_client : public rfcomm_client {
  private:
    fd_wrapper fd;

  public:
    rfcomm_ep_t get_local_ep() override {
      return fd.get_local_ep<rfcomm_ep_t>();
    }
    rfcomm_ep_t get_remote_ep() override {
      return fd.get_remote_ep<rfcomm_ep_t>();
    }

  public:
    void send(nu::data_const_ref b) override {
      fd.write(b);
    }

    nu::cancellable<size_t> receive(nu::data_ref b) override {
      return fd.receive_c(b);
    }

  public:
    _rfcomm_client(fd_wrapper fd) : fd{std::move(fd)} {}

  public:
    static nu::cancellable<std::unique_ptr<_rfcomm_client>> connect(rfcomm_ep_t remote);
  };

  nu::cancellable<std::unique_ptr<rfcomm_client>> rfcomm_host::connect(rfcomm_ep_t remote) {
    nu::cancellable<std::unique_ptr<fd_wrapper>> c = fd_wrapper::connect_c<decltype(remote)>(rfcomm_ep_t{local_addr, 0}, remote,
                                                     af<bluetooth::address>(), SOCK_STREAM, BTPROTO_RFCOMM);
    return c.map<std::unique_ptr<rfcomm_client>>([](std::unique_ptr<fd_wrapper> x) {
      return std::make_unique<_rfcomm_client>(std::move(*x));
    });
  }

  class rfcomm_server : public rfcomm_server_t {
  private:
    fd_wrapper fd;

  public:
    nu::cancellable<std::unique_ptr<rfcomm_client>> accept() {
      auto c = fd.accept_c();
      return c.template map<std::unique_ptr<rfcomm_client>>([](std::unique_ptr<fd_wrapper> x) {
        return std::make_unique<_rfcomm_client>(std::move(*x));
      });
    }
    rfcomm_ep_t get_ep() {
      return fd.get_local_ep<rfcomm_ep_t>();
    }

  public:
    rfcomm_server(decltype(fd) _fd) : fd{std::move(_fd)} {}
  };

  std::unique_ptr<rfcomm_server_t> rfcomm_host::listen(rfcomm_port_t port) {
    fd_wrapper fd = ::socket(af<bluetooth::address>(), SOCK_STREAM, BTPROTO_RFCOMM);
    if (!fd.bind<rfcomm_ep_t>({local_addr, port}))
      throw std::runtime_error("Could not bind to requested port");
    fd.listen();
    fd.set_fcntl_flag(O_NONBLOCK);
    return std::make_unique<rfcomm_server>(std::move(fd));
  }

  rfcomm_host::rfcomm_host(bluetooth::address addr) : local_addr{std::move(addr)} {}

  class bt_scanner final : public scanner<bluetooth::address> {
  public:
    fd_wrapper fd = ::hci_open_dev(::hci_get_route(nullptr));

  public:
    std::set<bluetooth::address> get_scanned() {
      std::cout << "Called" << std::endl;

      std::set<bluetooth::address> ret;

      std::array<uint8_t, HCI_MAX_EVENT_SIZE> buf;

      size_t len;

      while ((len = fd.read(buf)) == 0);
      std::cout << "Got one!" << std::endl;

      ::evt_le_meta_event& meta_event = *reinterpret_cast<::evt_le_meta_event*>(buf.data());

      //if (meta_event.subevent != EVT_LE_ADVERTISING_REPORT)
      //  continue;

      ::le_advertising_info& inf = *reinterpret_cast<::le_advertising_info*>(meta_event.data + 1);

      bluetooth::address addr;
      std::copy(inf.bdaddr.b, inf.bdaddr.b + addr.size(), addr.data());

      std::cout << nu::hex_encode(addr) << std::endl;

      ret.insert(addr);

      ::hci_le_set_scan_enable(fd, false, false, 1000);

      return ret;
    }

    virtual nu::cancellable<std::set<bluetooth::address>> scan() override {
      nu::cancellable_provider<int> provider;

      // Stop any previous scan
      ::hci_le_set_scan_enable(fd, 0, 0, 10000);

      if (::hci_le_set_scan_parameters(fd, 0x01, htobs(0x10), htobs(0x10),
                                       LE_PUBLIC_ADDRESS, true, 10000) < 0)
        throw std::runtime_error("Could not set bluetooth scan params");

      ::hci_filter filter_flags;
      ::hci_filter_clear(&filter_flags);
      //::hci_filter_set_ptype(HCI_EVENT_PKT, &filter_flags);
      ::hci_filter_set_event(EVT_LE_META_EVENT, &filter_flags);
      if (setsockopt(fd, SOL_HCI, HCI_FILTER, &filter_flags, sizeof(filter_flags)) < 0)
        throw std::runtime_error("Could not set bluetooth filter flags");

      if (::hci_le_set_scan_enable(fd, true, false, 10000) < 0)
        throw std::runtime_error("Could not start bluetooth scan params");
      
      return nu::cancellable<std::set<bluetooth::address>>::external(std::bind(&bt_scanner::get_scanned, this));
    }

  public:
    bt_scanner() {
      //fd.set_fcntl_flag(O_NONBLOCK);
    }
  };

  std::unique_ptr<scanner<bluetooth::address>> get_scanner() {
    return std::make_unique<bt_scanner>();
  }

}
