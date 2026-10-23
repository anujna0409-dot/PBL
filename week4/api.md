# API Contract 

Base URL: http://localhost:8080 . POST bodies are plain text (whitespace-separated integers), content type does not matter.
Every route returns the SAME JSON object, so the UI needs only one render function.

| Route | Method | Body | Purpose |
|---|---|---|---|
| /api/setup | POST | `n m` + Available[m] + Allocation[n*m] + Max[n*m] | Create system |
| /api/request | POST | `p r0 r1 ... r(m-1)` | Banker's request (grant / deny / wait) |
| /api/safety | GET | - | Run safety algorithm |
| /api/detect | GET | - | Run deadlock detection |
| /api/recover | GET | - | Terminate one victim, retry waiting requests |
| /api/status | GET | - | Current state |

Setup example (2 processes, 2 resources): `2 2  0 0  1 0 0 1  1 1 1 1`

## Response shape
```json
{
  "message": "text for the log panel",
  "state": "SAFE | UNSAFE | DEADLOCKED | NOT_SET",
  "n": 5, "m": 3,
  "total": [10,5,7],
  "available": [3,3,2],
  "allocation": [[0,1,0], ...],
  "max": [[7,5,3], ...],
  "need": [[7,4,3], ...],
  "requests": [[0,0,0], ...],
  "active": [1,1,1,1,1],
  "safeSequence": [1,3,4,0,2],
  "deadlocked": [],
  "utilization": {"overall": 63.6, "perResource": [70.0,40.0,71.4]},
  "edges": [{"from":"R1","to":"P0","type":"alloc","count":1},
            {"from":"P2","to":"R0","type":"request","count":1}]
}
```
- `active[i] = 0` means process i was terminated by recovery (grey it out).
- `edges` is the Resource Allocation Graph: `alloc` = Resource -> Process, `request` = Process -> Resource.
- Colours: SAFE green, UNSAFE orange, DEADLOCKED red.

## Module contract (common.h)
Member 2 owns banker.c: compute_need, is_safe, request_resources, utilization.
Member 3 owns detect.c: detect_deadlock, recover_one. Signatures must not change without telling the leader.
