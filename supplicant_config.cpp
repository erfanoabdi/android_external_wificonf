/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char kSupplicantConfigTemplatePath[] =
    "/etc/wifi/wpa_supplicant.conf";
const char kSupplicantConfigFile[] = "/data/misc/wifi/wpa_supplicant.conf";
const char kP2pConfigFile[] = "/data/misc/wifi/p2p_supplicant.conf";
constexpr mode_t kConfigFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

namespace {

int ensure_config_file_exists(const char* config_file) {
  char buf[2048];
  int srcfd, destfd;
  int nread;
  int ret;
  std::string templatePath;

  ret = access(config_file, R_OK | W_OK);
  if ((ret == 0) || (errno == EACCES)) {
    if ((ret != 0) && (chmod(config_file, kConfigFileMode) != 0)) {
      LOG(ERROR) << "Cannot set RW to \"" << config_file << "\": "
                 << strerror(errno);
      return false;
    }
    return true;
  } else if (errno != ENOENT) {
    LOG(ERROR) << "Cannot access \"" << config_file << "\": "
               << strerror(errno);
    return false;
  }

  std::string configPathSystem =
      std::string("/system") + std::string(kSupplicantConfigTemplatePath);
  std::string configPathVendor =
      std::string("/vendor") + std::string(kSupplicantConfigTemplatePath);
  srcfd = TEMP_FAILURE_RETRY(open(configPathSystem.c_str(), O_RDONLY));
  templatePath = configPathSystem;
  if (srcfd < 0) {
    int errnoSystem = errno;
    srcfd = TEMP_FAILURE_RETRY(open(configPathVendor.c_str(), O_RDONLY));
    templatePath = configPathVendor;
    if (srcfd < 0) {
      int errnoVendor = errno;
      LOG(ERROR) << "Cannot open \"" << configPathSystem << "\": "
                 << strerror(errnoSystem);
      LOG(ERROR) << "Cannot open \"" << configPathVendor << "\": "
                 << strerror(errnoVendor);
      return false;
    }
  }

  destfd = TEMP_FAILURE_RETRY(open(config_file,
                                   O_CREAT | O_RDWR,
                                   kConfigFileMode));
  if (destfd < 0) {
    close(srcfd);
    LOG(ERROR) << "Cannot create \"" << config_file << "\": "
               << strerror(errno);
    return false;
  }

  while ((nread = TEMP_FAILURE_RETRY(read(srcfd, buf, sizeof(buf)))) != 0) {
    if (nread < 0) {
      LOG(ERROR) << "Error reading \"" << templatePath
                 << "\": " << strerror(errno);
      close(srcfd);
      close(destfd);
      unlink(config_file);
      return false;
    }
    TEMP_FAILURE_RETRY(write(destfd, buf, nread));
  }

  close(destfd);
  close(srcfd);

  /* chmod is needed because open() didn't set permisions properly */
  if (chmod(config_file, kConfigFileMode) < 0) {
    LOG(ERROR) << "Error changing permissions of " << config_file
               << " to 0660: " << strerror(errno);
    unlink(config_file);
    return false;
  }

  return true;
}

}  // namespace

int main(int argc, char** argv) {
  /* Before starting the daemon, make sure its config file exists */
  if (ensure_config_file_exists(kSupplicantConfigFile) < 0) {
    LOG(ERROR) << "Wi-Fi will not be enabled";
    return -1;
  }

  /*
   * Some devices have another configuration file for the p2p interface.
   * However, not all devices have this, and we'll let it slide if it
   * is missing.  For devices that do expect this file to exist,
   * supplicant will refuse to start and emit a good error message.
   * No need to check for it here.
   */
  (void)ensure_config_file_exists(kP2pConfigFile);

  return 0;
}
