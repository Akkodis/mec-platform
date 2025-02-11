# MEC APPLICATIONS Development

## Introduction

This repository contains  demo CCAM applications in the MEC Platform of 5GMETA (using 5GMETA HW resources instead of deploying it in the Third-Party premises) will reduce the latencies of the service.      
This demo consists of 3 componens:
- **sender**: an example sender, publish data in the platform with a timestamp in the message properties;
- **ccam** : an example consumer from the 5GMETa platform, that computes the latency of the message received and exporting it in prometheus;
- **llccam**: performs the same operations of the **ccam**, but is deployed as a pipeline in the MEC platform using the chart in the **deploy/helm** folder. 


## Credits

* Federico Princiotto ([federico.princiotto@linksfoundation.com](mailto:federico.princiotto@linksfoundation.com))

## License

Copyright : Copyright 2022 LINKS

License : EUPL 1.2 ([https://eupl.eu/1.2/en/](https://eupl.eu/1.2/en/))

The European Union Public Licence (EUPL) is a copyleft free/open source software license created on the initiative of and approved by the European Commission in 23 official languages of the European Union.

Licensed under the EUPL License, Version 1.2 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at [https://eupl.eu/1.2/en/](https://eupl.eu/1.2/en/)

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
