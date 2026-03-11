# Agnocast Wrapper Migration Status

### 対象ノードと適用状況

| ノード | パッケージ | Topic名 | Pub/Sub | Agnocast適用 | 備考 |
|--------|-----------|---------|---------|-------------|------|
| `voxel_based_compare_map_filter` | `autoware_compare_map_segmentation` | `/perception/object_recognition/detection/pointcloud_map_filtered/pointcloud` | Pub | 適用済 | `compute_publish` override による回避策あり (後述) |
| `obstacle_pointcloud_based_validator` | `autoware_detected_object_validation` | `/perception/object_recognition/detection/pointcloud_map_filtered/pointcloud` | Sub | 適用済 | `message_filters::Subscriber` を agnocast wrapper 版に置換 |
| `shape_estimation` | `autoware_shape_estimation` | `/perception/object_recognition/detection/clustering/objects_with_feature` | Pub | 適用済 | |
| `detected_object_feature_remover` | `autoware_detected_object_feature_remover` | `/perception/object_recognition/detection/clustering/objects_with_feature` | Sub | 適用済 | |
| `detection_by_tracker_node` | `autoware_detection_by_tracker` | `/perception/object_recognition/detection/clustering/objects_with_feature` | Sub | 適用済 | `rclcpp::Node` 継承のためフリー関数 + `SingleThreadedAgnocast` で対応 |
| `lidar_centerpoint` | `autoware_lidar_centerpoint` | `/perception/object_recognition/detection/centerpoint/objects` | Pub | 適用済 | `rclcpp::Node` 継承 + `SingleThreadedAgnocast`。CudaBlackboardSubscriber はそのまま |
| `voxel_grid_based_euclidean_cluster_node` | `autoware_euclidean_cluster` | — | Pub/Sub | 適用済 | `AgnocastOnlySingleThreaded`。従来は `pointcloud_container` にコンポーネントとしてロードされていたが、`ENABLE_AGNOCAST=1` 時はスタンドアロンノードとして分離。`use_low_height_cropbox=true` の場合、`low_height_crop_box_filter` はコンポーネントとして `pointcloud_container` に残す |

### `voxel_based_compare_map_filter` — Publisher 側の制約と回避策

#### 問題

`voxel_based_compare_map_filter` は `autoware::pointcloud_preprocessor::Filter` base class を継承しており、
base class の `compute_publish` メソッド内で `pub_output_` (rclcpp publisher) を使って publish している。

agnocast publisher (`agnocast_pub_output_`) で publish するには `compute_publish` を override する必要があるが、
base class の `convert_output_costly` が `std::unique_ptr<PointCloud2> &` を受け取る virtual メソッドであるため、
`ALLOCATE_OUTPUT_MESSAGE_UNIQUE` (agnocast 共有メモリからの確保) を使った zero-copy publish ができない。

#### 現状の回避策

`compute_publish` を override し、base class と同じロジックで filter → convert → publish を行うが、
publish 先を `agnocast_pub_output_->publish(*output)` (const ref によるコピー渡し) としている。

```cpp
void VoxelBasedCompareMapFilterComponent::compute_publish(
  const PointCloud2ConstPtr & input, const IndicesPtr & indices)
{
  auto output = std::make_unique<PointCloud2>();
  filter(input, indices, *output);
  if (!convert_output_costly(output)) return;
  output->header.stamp = input->header.stamp;
  agnocast_pub_output_->publish(*output);  // コピー渡し (zero-copy ではない)
  published_time_publisher_->publish_if_subscribed(pub_output_, input->header.stamp);
}
```

#### 今後の改善

zero-copy を実現するには、`Filter` base class の `compute_publish` / `convert_output_costly` のシグネチャを
`AUTOWARE_MESSAGE_UNIQUE_PTR` 対応にリファクタする必要がある。影響範囲が大きいため、別タスクとして対応予定。
