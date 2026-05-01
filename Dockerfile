# Sử dụng image chính thức từ Espressif
FROM espressif/idf:release-v5.5

# Cài đặt các phụ kiện cần thiết cho Matter
RUN apt-get update && \
    apt-get install -y \
    git \
    python3 \
    python3-pip \
    python3-venv \
    python3-full \
    python3-setuptools \
    libpcap-dev \
    libdbus-1-dev \
    libavahi-client-dev \
    libavahi-common-dev \
    libssl-dev \
    libglib2.0-dev \
    libreadline-dev \
    && rm -rf /var/lib/apt/lists/*

# Thiết lập biến môi trường để cho phép cài đặt package bằng pip vào hệ thống (Python 3.12+)
ENV PIP_BREAK_SYSTEM_PACKAGES=1

# Thiết lập thư mục làm việc cho SDKs
WORKDIR /opt/espressif

# Clone esp-matter SDK với shallow clone để tiết kiệm dung lượng
RUN git clone --depth 1 --recursive https://github.com/espressif/esp-matter.git

# Cài đặt esp-matter (Bootstrap)
WORKDIR /opt/espressif/esp-matter
RUN ./install.sh

# Thiết lập biến môi trường
ENV ESP_MATTER_PATH=/opt/espressif/esp-matter

# Quay lại thư mục dự án
WORKDIR /project

# Mặc định khi chạy container sẽ khởi động môi trường idf
ENTRYPOINT [ "/opt/espressif/entrypoint.sh" ]
CMD [ "idf.py", "build" ]
