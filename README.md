## GStreamer for LookoutForVision - `aws.greengrass.labs.lookoutvision.GStreamer`

This AWS IoT Greengrass component provides a GStreamer plugin that enables customers to perform Lookout for Vision 
anomaly detection in their custom GStreamer pipelines. This plugin takes the image buffer and sends it to Lookout for 
Vision Edge Agent and gets back the inference result as response.

This GStreamer plugin is essentially a client to Lookout for Vision Edge Agent running on edge device. The Edge Agent 
serves inferences over gRPC protocol. The corresponding gRPC client is built into the plugin's shared object binary. 
This plugin receives the image buffer at its sink pad, gets inference results for the image buffer over gRPC 
request/response and attaches the inference results to the image buffer before propagating it downstream through its 
source pad.

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

This project is licensed under the Apache-2.0 License.

