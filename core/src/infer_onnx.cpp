#if defined(FOVEA_HAS_ONNX)

#include "fovea/coco_labels.hpp"
#include "fovea/infer.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fovea {

namespace {

struct Letterbox {
    float scale = 1.0F;
    float pad_x = 0.0F;
    float pad_y = 0.0F;
};

struct Candidate {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    float confidence = 0.0F;
    int class_id = 0;
};

bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

Letterbox compute_letterbox(int width, int height, int target) {
    const float scale =
        std::min(static_cast<float>(target) / static_cast<float>(width),
                 static_cast<float>(target) / static_cast<float>(height));
    const float pad_x = (static_cast<float>(target) - static_cast<float>(width) * scale) * 0.5F;
    const float pad_y = (static_cast<float>(target) - static_cast<float>(height) * scale) * 0.5F;
    return Letterbox{scale, pad_x, pad_y};
}

std::vector<float> preprocess_image(const ImageBuffer& image, int target) {
    const Letterbox letterbox = compute_letterbox(image.width, image.height, target);
    std::vector<float> tensor(static_cast<std::size_t>(3 * target * target), 0.114F);

    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const int dst_x = static_cast<int>(std::round(x * letterbox.scale + letterbox.pad_x));
            const int dst_y = static_cast<int>(std::round(y * letterbox.scale + letterbox.pad_y));
            if (dst_x < 0 || dst_x >= target || dst_y < 0 || dst_y >= target) {
                continue;
            }

            const std::size_t src_index =
                static_cast<std::size_t>((y * image.width + x) * image.channels);
            const std::size_t dst_index =
                static_cast<std::size_t>(dst_y * target + dst_x);

            tensor[dst_index] = static_cast<float>(image.pixels[src_index]) / 255.0F;
            tensor[static_cast<std::size_t>(target * target) + dst_index] =
                static_cast<float>(image.pixels[src_index + 1]) / 255.0F;
            tensor[static_cast<std::size_t>(2 * target * target) + dst_index] =
                static_cast<float>(image.pixels[src_index + 2]) / 255.0F;
        }
    }

    return tensor;
}

float intersection_over_union(const Candidate& a, const Candidate& b) {
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);

    const float intersection = std::max(0.0F, x2 - x1) * std::max(0.0F, y2 - y1);
    const float union_area = a.width * a.height + b.width * b.height - intersection;
    return union_area <= 0.0F ? 0.0F : intersection / union_area;
}

std::vector<Candidate> non_max_suppression(std::vector<Candidate> candidates,
                                           float threshold) {
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& lhs, const Candidate& rhs) {
                  return lhs.confidence > rhs.confidence;
              });

    std::vector<Candidate> kept;
    std::vector<bool> suppressed(candidates.size(), false);

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (suppressed[i]) {
            continue;
        }
        kept.push_back(candidates[i]);
        for (std::size_t j = i + 1; j < candidates.size(); ++j) {
            if (suppressed[j]) {
                continue;
            }
            if (candidates[i].class_id != candidates[j].class_id) {
                continue;
            }
            if (intersection_over_union(candidates[i], candidates[j]) > threshold) {
                suppressed[j] = true;
            }
        }
    }

    return kept;
}

BoundingBox remap_bbox(const Candidate& candidate, const Letterbox& letterbox) {
    const float x = (candidate.x - letterbox.pad_x) / letterbox.scale;
    const float y = (candidate.y - letterbox.pad_y) / letterbox.scale;
    const float width = candidate.width / letterbox.scale;
    const float height = candidate.height / letterbox.scale;
    return BoundingBox{x, y, width, height};
}

}  // namespace

struct InferEngine::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "fovea"};
    Ort::SessionOptions session_options;
    std::unique_ptr<Ort::Session> session;
    std::string input_name;
    std::string output_name;

    explicit Impl(const std::string& model_path) {
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        session = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);

        Ort::AllocatorWithDefaultOptions allocator;
        {
            auto name = session->GetInputNameAllocated(0, allocator);
            input_name = name.get();
        }
        {
            auto name = session->GetOutputNameAllocated(0, allocator);
            output_name = name.get();
        }
    }
};

InferEngine::InferEngine(InferConfig config)
    : config_(std::move(config)), impl_(nullptr) {}

void run_onnx_inference(const InferEngine& engine, SceneFrame& frame) {
    const InferConfig& config = engine.config_;
  if (!file_exists(config.model_path)) {
        throw std::runtime_error("ONNX model not found: " + config.model_path);
    }

    std::unique_ptr<InferEngine::Impl>& impl = engine.impl_;
    if (!impl) {
        impl = std::make_unique<InferEngine::Impl>(config.model_path);
    }

    const Letterbox letterbox =
        compute_letterbox(frame.image.width, frame.image.height, config.input_size);
    const std::vector<float> input_tensor =
        preprocess_image(frame.image, config.input_size);

    const std::array<std::int64_t, 4> input_shape = {1, 3, config.input_size, config.input_size};
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_value = Ort::Value::CreateTensor<float>(
        memory_info, const_cast<float*>(input_tensor.data()), input_tensor.size(),
        input_shape.data(), input_shape.size());

    const char* input_names[] = {impl->input_name.c_str()};
    const char* output_names[] = {impl->output_name.c_str()};

    auto outputs = impl->session->Run(Ort::RunOptions{nullptr}, input_names, &input_value, 1,
                                      output_names, 1);

    float* output_data = outputs[0].GetTensorMutableData<float>();
    const auto output_info = outputs[0].GetTensorTypeAndShapeInfo();
    const std::vector<std::int64_t> output_shape = output_info.GetShape();

    if (output_shape.size() != 3) {
        throw std::runtime_error("Unexpected YOLO output rank");
    }

    const std::int64_t attributes = output_shape[1];
    const std::int64_t candidates = output_shape[2];
    const int num_classes = static_cast<int>(attributes - 4);

    std::vector<Candidate> parsed;
    parsed.reserve(static_cast<std::size_t>(candidates));

    for (std::int64_t i = 0; i < candidates; ++i) {
        const float cx = output_data[i];
        const float cy = output_data[candidates + i];
        const float width = output_data[2 * candidates + i];
        const float height = output_data[3 * candidates + i];

        int best_class = 0;
        float best_score = 0.0F;
        for (int class_id = 0; class_id < num_classes; ++class_id) {
            const float score = output_data[(4 + class_id) * candidates + i];
            if (score > best_score) {
                best_score = score;
                best_class = class_id;
            }
        }

        if (best_score < config.confidence_threshold) {
            continue;
        }

        Candidate candidate{};
        candidate.x = cx - width * 0.5F;
        candidate.y = cy - height * 0.5F;
        candidate.width = width;
        candidate.height = height;
        candidate.confidence = best_score;
        candidate.class_id = best_class;
        parsed.push_back(candidate);
    }

    const std::vector<Candidate> filtered =
        non_max_suppression(std::move(parsed), config.nms_threshold);

    frame.detections.clear();
    frame.detections.reserve(filtered.size());

    std::uint32_t next_id = 1;
    for (const auto& candidate : filtered) {
        Detection detection{};
        detection.id = next_id++;
        detection.label = std::string(kCocoLabels[static_cast<std::size_t>(candidate.class_id)]);
        detection.confidence = candidate.confidence;
        detection.bbox = remap_bbox(candidate, letterbox);
        frame.detections.push_back(detection);
    }
}

InferEngine::~InferEngine() = default;

}  // namespace fovea

#endif
