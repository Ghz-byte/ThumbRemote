#include "dual_profile_store.h"
#include <nvs.h>
namespace dual_remote {
namespace {
class NvsProfileIo final:public ProfileIo {
  Profile read() override {
    Profile result;nvs_handle_t handle;
    auto rc=nvs_open("tr_dual",NVS_READONLY,&handle);
    if(rc==ESP_ERR_NVS_NOT_FOUND)return result;
    result.status=ProfileStatus::Fault;
    if(rc!=ESP_OK)return result;
    size_t length=0;rc=nvs_get_blob(handle,"app_peer",nullptr,&length);
    if(rc==ESP_ERR_NVS_NOT_FOUND){nvs_close(handle);result.status=ProfileStatus::Missing;return result;}
    if(rc!=ESP_OK || length!=16){nvs_close(handle);return result;}
    ProfileBytes bytes{};rc=nvs_get_blob(handle,"app_peer",bytes.data(),&length);nvs_close(handle);
    return rc==ESP_OK?decodeProfile(bytes.data(),length):result;
  }
  bool write(const ProfileBytes& bytes) override {
    nvs_handle_t handle;
    if(nvs_open("tr_dual",NVS_READWRITE,&handle)!=ESP_OK)return false;
    auto rc=nvs_set_blob(handle,"app_peer",bytes.data(),bytes.size());
    if(rc==ESP_OK)rc=nvs_commit(handle);
    nvs_close(handle);return rc==ESP_OK;
  }
} nvsIo;
}
ProfileStore::ProfileStore():io_(nvsIo){}
}
