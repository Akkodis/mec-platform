#!/usr/bin/env python3
# Copyright 2022 Canonical Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# For those usages not covered by the Apache License, Version 2.0 please
# contact: legal@canonical.com
#
# To get in touch with the maintainers, please contact:
# osm-charmers@lists.launchpad.net
#
#
# Learn more at: https://juju.is/docs/sdk

"""Ro library.

This [library](https://juju.is/docs/sdk/libraries) implements both sides of the
`ro` [interface](https://juju.is/docs/sdk/relations).

The *provider* side of this interface is implemented by the
[osm-ro Charmed Operator](https://charmhub.io/osm-ro).

Any Charmed Operator that *requires* RO for providing its
service should implement the *requirer* side of this interface.

In a nutshell using this library to implement a Charmed Operator *requiring*
RO would look like

```
$ charmcraft fetch-lib charms.osm_ro.v0.ro
```

`metadata.yaml`:

```
requires:
  ro:
    interface: ro
    limit: 1
```

`src/charm.py`:

```
from charms.osm_ro.v0.ro import RoRequires
from ops.charm import CharmBase


class MyCharm(CharmBase):

    def __init__(self, *args):
        super().__init__(*args)
        self.ro = RoRequires(self)
        self.framework.observe(
            self.on["ro"].relation_changed,
            self._on_ro_relation_changed,
        )
        self.framework.observe(
            self.on["ro"].relation_broken,
            self._on_ro_relation_broken,
        )
        self.framework.observe(
            self.on["ro"].relation_broken,
            self._on_ro_broken,
        )

    def _on_ro_relation_broken(self, event):
        # Get RO host and port
        host: str = self.ro.host
        port: int = self.ro.port
        # host => "osm-ro"
        # port => 9999

    def _on_ro_broken(self, event):
        # Stop service
        # ...
        self.unit.status = BlockedStatus("need ro relation")
```

You can file bugs
[here](https://osm.etsi.org/bugzilla/enter_bug.cgi), selecting the `devops` module!
"""
from typing import Optional

from ops.charm import CharmBase, CharmEvents
from ops.framework import EventBase, EventSource, Object
from ops.model import Relation


# The unique Charmhub library identifier, never change it
LIBID = "a34c3331a43f4f6db2b1499ff4d1390d"

# Increment this major API version when introducing breaking changes
LIBAPI = 0

# Increment this PATCH version before using `charmcraft publish-lib` or reset
# to 0 if you are raising the major API version
LIBPATCH = 1

RO_HOST_APP_KEY = "host"
RO_PORT_APP_KEY = "port"


class RoRequires(Object):  # pragma: no cover
    """Requires-side of the Ro relation."""

    def __init__(self, charm: CharmBase, endpoint_name: str = "ro") -> None:
        super().__init__(charm, endpoint_name)
        self.charm = charm
        self._endpoint_name = endpoint_name

    @property
    def host(self) -> str:
        """Get ro hostname."""
        relation: Relation = self.model.get_relation(self._endpoint_name)
        return (
            relation.data[relation.app].get(RO_HOST_APP_KEY)
            if relation and relation.app
            else None
        )

    @property
    def port(self) -> int:
        """Get ro port number."""
        relation: Relation = self.model.get_relation(self._endpoint_name)
        return (
            int(relation.data[relation.app].get(RO_PORT_APP_KEY))
            if relation and relation.app
            else None
        )


class RoProvides(Object):
    """Provides-side of the Ro relation."""

    def __init__(self, charm: CharmBase, endpoint_name: str = "ro") -> None:
        super().__init__(charm, endpoint_name)
        self._endpoint_name = endpoint_name

    def set_host_info(self, host: str, port: int, relation: Optional[Relation] = None) -> None:
        """Set Ro host and port.

        This function writes in the application data of the relation, therefore,
        only the unit leader can call it.

        Args:
            host (str): Ro hostname or IP address.
            port (int): Ro port.
            relation (Optional[Relation]): Relation to update.
                                           If not specified, all relations will be updated.

        Raises:
            Exception: if a non-leader unit calls this function.
        """
        if not self.model.unit.is_leader():
            raise Exception("only the leader set host information.")

        if relation:
            self._update_relation_data(host, port, relation)
            return

        for relation in self.model.relations[self._endpoint_name]:
            self._update_relation_data(host, port, relation)

    def _update_relation_data(self, host: str, port: int, relation: Relation) -> None:
        """Update data in relation if needed."""
        relation.data[self.model.app][RO_HOST_APP_KEY] = host
        relation.data[self.model.app][RO_PORT_APP_KEY] = str(port)
