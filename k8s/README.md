# Kubernetes manifests

Server_Design.md step 8 ("Deploy to K3s locally"). One Deployment + Service pair per tier,
mirroring `docker-compose.yml`'s coverage. Every gameserver/gateway/matchmaker/game-allocator
image is scaled by replica count, not by duplicating manifests - see each file's own header
comment for why that's actually true here (dynamic shard discovery over NATS, replacing what
used to be a static shard list).

No registry is assumed - these are meant to run against a local K3s cluster
(`curl -sfL https://get.k3s.io | sh -`), which is "how we'd practice the K8s workflow locally"
per `Server_Design.md`'s own framing, not a production target.

## Build and import images

K3s embeds containerd, not the Docker daemon, so a locally `docker build`-ed image isn't
visible to it until explicitly imported:

```sh
# From the repo root, one build + save + import per Dockerfile stage:
for stage in server:kung-fu-chess/server ws-gateway:kung-fu-chess/ws-gateway \
             api-gateway:kung-fu-chess/api-gateway matchmaker:kung-fu-chess/matchmaker \
             game-allocator:kung-fu-chess/game-allocator; do
    target=${stage%%:*}; image=${stage##*:}
    docker build --target "$target" -t "$image:latest" .
    docker save "$image:latest" -o "/tmp/$target.tar"
    sudo k3s ctr images import "/tmp/$target.tar"
done
```

Every Deployment sets `imagePullPolicy: IfNotPresent` for exactly this reason - it'll use the
imported image instead of trying (and failing) to pull from a registry.

## Apply order

Data tier first, then everything that depends on it - `kubectl apply` doesn't retry a failed
dependency automatically, though the readiness probes mean a slightly-out-of-order apply
mostly self-heals once the dependency comes up:

```sh
kubectl apply -f k8s/postgres.yaml -f k8s/redis.yaml -f k8s/nats.yaml
kubectl wait --for=condition=ready pod -l app=postgres --timeout=60s
kubectl wait --for=condition=ready pod -l app=redis --timeout=60s
kubectl wait --for=condition=ready pod -l app=nats --timeout=60s

kubectl apply -f k8s/gameserver.yaml -f k8s/matchmaker.yaml -f k8s/game-allocator.yaml
kubectl apply -f k8s/ws-gateway.yaml -f k8s/api-gateway.yaml
```

## Verify

```sh
kubectl get pods                     # 2 gameserver, 2 ws-gateway, 2 api-gateway, 2 matchmaker,
                                      # 2 game-allocator, 1 each of postgres/redis/nats
kubectl get svc ws-gateway api-gateway   # note the LoadBalancer EXTERNAL-IP (K3s's built-in
                                          # ServiceLB assigns one on a local single-node
                                          # cluster - see ws-gateway.yaml's own comment)
```

Then point a real client (or this repo's `test_play.js`/`test_timeout_and_room.js` scratchpad
scripts, adjusted for the LoadBalancer IP instead of `localhost`) at the `ws-gateway`/
`api-gateway` external IPs and confirm two players still match across different `gameserver`
pods, the same regression check used for the Docker Compose version of this change.

To prove scaling is real, not just declared: `kubectl scale deployment/gameserver --replicas=3`
and confirm a third pod actually receives matched traffic.

## A real bug found deploying this for the first time

Kubernetes auto-injects a `REDIS_PORT` env var into every pod for each Service named "redis" in
scope ("Service Links" - `tcp://<clusterIP>:6379`), colliding with this app's own `REDIS_PORT`
(a plain port number, or unset to fall back to 6379 - see `server_main.cpp`). Without a fix,
`std::atoi("tcp://...")` silently parses to 0 and every Redis connection fails with "Connection
refused" - `gameserver`/`game-allocator`/`api-gateway` all connect to Redis, so all three
CrashLoopBackOff'd. Fixed with `enableServiceLinks: false` on those three Deployments' pod specs
(disables the auto-injection entirely - the app's own env vars are unaffected). Confirmed this
by running the actual `Server` binary manually inside a debug pod using the real image: a raw
TCP connect and `redis-cli` both succeeded, isolating the bug to that one shadowed env var.

## Known limitations (see individual files for detail)

- `postgres` is a Deployment + PVC, not a StatefulSet - fine at `replicas: 1`, not the more
  correct primitive `Server_Design.md` itself names for a database tier.
- `gameserver`/`ws-gateway`/`api-gateway` probes are `tcpSocket` only - none of the three
  binaries expose an HTTP health endpoint today, so this can't distinguish "briefly slow" from
  "should stop receiving new rooms" the way `Server_Design.md` §7's fuller readiness-probe
  guidance describes.
