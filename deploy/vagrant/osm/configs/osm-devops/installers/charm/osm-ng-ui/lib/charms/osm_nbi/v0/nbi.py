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

"""Nbi library.

This [library](https://juju.is/docs/sdk/libraries) implements both sides of the
`nbi` [interface](https://juju.is/docs/sdk/relations).

The *provider* side of this interface is implemented by the
[osm-nbi Charmed Operator](https://charmhub.io/osm-nbi).

Any Charmed Operator that *requires* NBI for providing its
service should implement the *requirer* side of this interface.

In a nutshell using this library to implement a Charmed Operator *requiring*
NBI would look like

```
$ charmcraft fetch-lib charms.osm_nbi.v0.nbi
```

`metadata.yaml`:

```
requires:
  nbi:
    interface: nbi
    limit: 1
```

`src/charm.py`:

```
from charms.osm_nbi.v0.nbi import NbiRequires
from ops.charm import CharmBase


class MyCharm(CharmBase):

    def __init__(self, *args):
        super().__init__(*args)
        self.nbi = NbiRequires(self)
        self.framework.observe(
            self.on["nbi"].relation_changed,
            self._on_nbi_relation_changed,
        )
        self.framework.observe(
            self.on["nbi"].relation_broken,
            self._on_nbi_relation_broken,
        )
        self.framework.observe(
            self.on["nbi"].relation_broken,
            self._on_nbi_broken,
        )

    def _on_nbi_relation_broken(self, event):
        # Get NBI host and port
        host: str = self.nbi.host
        port: int = self.nbi.port
        # host => "osm-nbi"
        # port => 9999

    def _on_nbi_broken(self, event):
        # Stop service
        # ...
        self.unit.status = BlockedStatus("need nbi relation")
```

You can file bugs
[here](https://osm.etsi.org/bugzilla/enter_bug.cgi), selecting the `devops` module!
"""
from typing import Optional

from ops.charm import CharmBase, CharmEvents
from ops.framework import EventBase, EventSource, Object
from ops.model import Relation


# The unique Charmhub library identifier, never change it
LIBID = "8c888f7c869949409e12c16d78ec068b"

# Increment this major API version when introducing breaking changes
LIBAPI = 0

# Increment this PATCH version before using `charmcraft publish-lib` or reset
# to 0 if you are raising the major API version
LIBPATCH = 1

NBI_HOST_APP_KEY = "host"
NBI_PORT_APP_KEY = "port"


class NbiRequires(Object):  # pragma: no cover
    """Requires-side of the Nbi relation."""

    def __init__(self, charm: CharmBase, endpoint_name: str = "nbi") -> None:
        super().__init__(charm, endpoint_name)
        self.charm = charm
        self._endpoint_name = endpoint_name

    @property
    def host(self) -> str:
        """Get nbi hostname."""
        relation: Relation = self.model.get_relation(self._endpoint_name)
        return (
            relation.data[relation.app].get(NBI_HOST_APP_KEY)
            if relation and relation.app
            else None
        )

    @property
    def port(self) -> int:
        """Get nbi port number."""
        relation: Relation = self.model.get_relation(self._endpoint_name)
        return (
            int(relation.data[relation.app].get(NBI_PORT_APP_KEY))
            if relation and relation.app
            else None
        )


class NbiProvides(Object):
    """Provides-side of the Nbi relation."""

    def __init__(self, charm: CharmBase, endpoint_name: str = "nbi") -> None:
        super().__init__(charm, endpoint_name)
        self._endpoint_name = endpoint_name

    def set_host_info(self, host: str, port: int, relation: Optional[Relation] = None) -> None:
        """Set Nbi host and port.

        This function writes in the application data of the relation, therefore,
        only the unit leader can call it.

        Args:
            host (str): Nbi hostname or IP address.
            port (int): Nbi port.
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
        relation.data[self.model.app][NBI_HOST_APP_KEY] = host
        relation.data[self.model.app][NBI_PORT_APP_KEY] = str(port)
