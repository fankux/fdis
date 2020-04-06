source conf.sh

#### disable swap
sudo swapoff -a

#### disable selinux
sudo setenforce 0

#### master init
http_proxy=http://${proxy}/ https_proxy=http://${proxy}/ no_proxy=127.0.0.1,localhost,${hostlist},10.244.0.0/16,10.96.0.0/12 \
  kubeadm init --pod-network-cidr=10.244.0.0/16 --control-plane-endpoint "k8s-api:8443" --upload-certs

#### install flannel
wget https://raw.githubusercontent.com/coreos/flannel/v0.12.0/Documentation/kube-flannel.yml -O kube-flannel.yml
kubectl apply -f kube-flannel.yml

#### make master schedule pods
kubectl taint nodes --all node-role.kubernetes.io/master-
