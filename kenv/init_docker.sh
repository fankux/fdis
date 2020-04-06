######## make docker use systemd
sudo echo '
{
        "exec-opts": ["native.cgroupdriver=systemd"],
        "log-driver": "json-file",
        "log-opts": {
                "max-size": "100m"
        },
        "storage-driver": "overlay2"
}
' >/etc/docker/daemon.json

sudo mkdir /etc/systemd/system/docker.service.d

########  set docker proxy
source conf.sh
sudo echo '
[Service]
Environment="HTTP_PROXY=http://'${proxy}'" "HTTPS_PROXY=http://'${proxy}'" "NO_PROXY=localhost,127.0.0.1,'${hostlist}',10.244.0.0/16,10.96.0.0/12"
' >/etc/systemd/system/docker.service.d/http-proxy.conf

sudo systemctl daemon-reload
sudo systemctl restart docker
