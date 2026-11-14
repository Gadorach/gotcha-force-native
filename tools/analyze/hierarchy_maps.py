"""Bounded hierarchy/permutation metadata audit,not transforms or ownership.

Links are (first child,next sibling,parent) with explicit -1 sentinels.
Traversal and active visit order are distinct inputs. Inactive mirror values
are retained verbatim rather than repaired into a full permutation.
"""


def audit_hierarchy_maps(links, traversal, active_order, mirror, *, max_nodes=4096):
    if (type(max_nodes) is not int or not 1 <= max_nodes <= 65536
            or not isinstance(links, (list, tuple)) or not 1 <= len(links) <= max_nodes):
        raise ValueError('bounded hierarchy extent')
    count = len(links)
    rows = []
    for row in links:
        if (not isinstance(row, (list, tuple)) or len(row) != 3
                or any(type(value) is not int or not -1 <= value < count for value in row)):
            raise ValueError('hierarchy link domain')
        rows.append(tuple(row))
    if (not isinstance(traversal, (list, tuple)) or len(traversal) != count
            or any(type(value) is not int for value in traversal)
            or sorted(traversal) != list(range(count))):
        raise ValueError('traversal must be a logical-node permutation')
    if (not isinstance(active_order, (list, tuple)) or len(active_order) > count
            or any(type(value) is not int or not 0 <= value < count for value in active_order)
            or len(set(active_order)) != len(active_order)):
        raise ValueError('active visit order domain/duplicate')
    if (not isinstance(mirror, (list, tuple)) or len(mirror) != count
            or any(type(value) is not int or not -1 <= value <= 0xffffffff for value in mirror)):
        raise ValueError('mirror extent/value domain')
    for node, (child, sibling, parent) in enumerate(rows):
        if child != -1 and rows[child][2] != node:
            raise ValueError('child parent mismatch')
        if sibling != -1 and rows[sibling][2] != parent:
            raise ValueError('sibling parent mismatch')
    root = traversal[0]
    if rows[root][2] != -1:
        raise ValueError('traversal root has a parent')
    seen = set()
    derived = []
    pending = [root]
    while pending:
        node = pending.pop()
        if node in seen:
            raise ValueError('hierarchy cycle or multiply visited node')
        seen.add(node)
        derived.append(node)
        child, sibling, _ = rows[node]
        if sibling != -1:
            pending.append(sibling)
        if child != -1:
            pending.append(child)
    if derived != list(traversal):
        raise ValueError('hierarchy coverage/traversal order mismatch')
    active = set(active_order)
    targets = [mirror[node] for node in active_order]
    if any(target not in active for target in targets) or len(set(targets)) != len(targets):
        raise ValueError('active mirror is foreign or not bijective')
    positions = [0] * count
    for position, node in enumerate(traversal):
        positions[node] = position
    return dict(
        root=root,
        traversal=list(traversal),
        traversal_position=positions,
        active_visits=[dict(node=node, traversal_position=positions[node], mirror_target=mirror[node],
                            mirror_traversal_position=positions[mirror[node]]) for node in active_order],
        unvisited_nodes=sorted(set(range(count)) - active),
        inactive_mirror_values=[dict(node=node, value=mirror[node]) for node in range(count) if node not in active],
        active_matches_filtered_traversal=list(active_order) == [node for node in traversal if node in active],
        mirror_involution_on_active=all(mirror[mirror[node]] == node for node in active_order),
        metadata_only_not_pose_frame_or_owner=True,
    )
