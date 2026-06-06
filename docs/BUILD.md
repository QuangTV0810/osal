# Hướng dẫn Build OSAL cho SoC RTS3917N

Tài liệu này hướng dẫn cách biên dịch chéo (cross-compile) thư viện Operating System Abstraction Layer (OSAL) cho nền tảng Realtek RTS3917N.

## 1. Yêu cầu hệ thống

- **Toolchain**: `asdk-10.3.1-a7-EL-5.4-u1.0-a32nh-220218`
- **Hệ điều hành mục tiêu**: Linux (ARM)
- **Phụ thuộc**: CMake (phiên bản 3.10 trở lên)

## 2. Các phương thức Build

### Cách 1: Sử dụng Script tự động (Khuyên dùng)

Bạn có thể sử dụng script đã được chuẩn bị sẵn để thực hiện toàn bộ quá trình build và install.

```bash
cd /home/quangtv/Workspace/SDK/Sources/Platform/osal
chmod +x scripts/build.sh
./scripts/build.sh
```

### Cách 2: Thực hiện thủ công

Nếu bạn muốn tùy chỉnh các thông số cấu hình, hãy thực hiện theo các bước sau:

1.  **Di chuyển đến thư mục gốc của OSAL:**
    ```bash
    cd /home/quangtv/Workspace/SDK/Sources/Platform/osal
    ```

2.  **Tạo thư mục build:**
    ```bash
    mkdir -p build-rts3917n && cd build-rts3917n
    ```

3.  **Cấu hình CMake:**
    ```bash
    cmake -DCMAKE_SYSTEM_NAME=Linux \
          -DCMAKE_SYSTEM_PROCESSOR=arm \
          -DCMAKE_C_COMPILER=/home/quangtv/Workspace/IPCam/Source/IP_Cam/toolchain/asdk-10.3.1-a7-EL-5.4-u1.0-a32nh-220218/bin/arm-linux-gcc \
          -DOSAL_SYSTEM_BSPTYPE=generic-linux \
          -DOSAL_SYSTEM_OSTYPE=posix \
          -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
          -DCMAKE_BUILD_TYPE=Release \
          ..
    ```

4.  **Biên dịch và cài đặt:**
    ```bash
    make -j$(nproc)
    make install
    ```

## 3. Kết quả đầu ra

Sau khi quá trình build hoàn tất, các sản phẩm sẽ được đặt tại:
`/home/quangtv/Workspace/SDK/Sources/Platform/osal/build-rts3917n/install`

Cấu trúc thư mục install bao gồm:
- `lib/`: Chứa các file thư viện tĩnh (`.a`).
- `include/`: Chứa các header files cần thiết để tích hợp vào ứng dụng.
- `lib/cmake/`: Các file cấu hình CMake để sử dụng OSAL trong các project khác qua `find_package()`.
