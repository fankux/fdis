
wget https://raw.githubusercontent.com/kubernetes/dashboard/v2.0.0-rc7/aio/deploy/recommended.yaml -O dashboard.yaml
kubectl apply -f dashboard.yaml

kubectl apply -f service_account.yaml

kubectl apply -f rolebind.yaml

# see http://127.0.0.1:8001/api/v1/namespaces/kubernetes-dashboard/services/https:kubernetes-dashboard:/proxy/#/overview?namespace=default
