wget https://get.helm.sh/helm-v2.16.5-linux-amd64.tar.gz -O helm.tar.gz && tar -zxf helm.tar.gz && mv linux-amd64/ helm/ && sudo mv helm/helm /usr/local/bin/

helm init

#### Create a service account:
kubectl create serviceaccount --namespace kube-system tiller

#### Bind the new service account to the cluster-admin role. This will give tiller admin access to the entire cluster:
kubectl create clusterrolebinding tiller-cluster-rule --clusterrole=cluster-admin --serviceaccount=kube-system:tiller

#### Deploy tiller and add the line serviceAccount: tiller to spec.template.spec:
kubectl patch deploy --namespace kube-system tiller-deploy -p '{"spec":{"template":{"spec":{"serviceAccount":"tiller"}}}}'

#### create the cluster role binding using:
kubectl apply -f rolebind-api.yaml

#### This CRB gives the kubelet-api user full access to the kubelet API.
#### Now we are ready to install metrics-server. Do this using:

helm install --name metrics-server stable/metrics-server  \
--namespace metrics --set args={"--kubelet-insecure-tls=true, --kubelet-preferred-address-types=InternalIP\,Hostname\,ExternalIP"}
