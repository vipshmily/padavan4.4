# 第一阶段，构建应用程序
FROM ubuntu:latest as msd_lite-builder

# 安装必要的构建工具和依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# 克隆 msd_lite 项目
RUN git clone --recursive https://github.com/TowayWei/msd_lite.git /msd_lite

# 构建 msd_lite
WORKDIR /msd_lite/build
RUN cmake .. && make -j$(nproc)

# 第二阶段，创建最终镜像，使用带有glibc的busybox版本
FROM busybox:glibc

# 从构建阶段复制构建产物到最终镜像中
COPY --from=msd_lite-builder /msd_lite/build/src/msd_lite /usr/bin/msd_lite

# 从构建阶段复制配置文件到最终镜像中
COPY --from=msd_lite-builder /msd_lite/build/msd_lite.conf.sample /etc/msd_lite.conf

# 确保 msd_lite 二进制文件具有执行权限
RUN chmod +x /usr/bin/msd_lite

# 暴露 msd_lite 使用的端口
EXPOSE 7088

# 设置容器启动时执行的命令
ENTRYPOINT ["/usr/bin/msd_lite"]
CMD ["-v", "-c", "/etc/msd_lite.conf"]
