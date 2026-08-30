#include "fovea/pipeline.hpp"

#include "fovea/capture.hpp"

#include <chrono>

namespace fovea {

namespace {

double elapsed_ms(const std::chrono::steady_clock::time_point& start,
                  const std::chrono::steady_clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

Pipeline::Pipeline() = default;

void Pipeline::reset_tracking() const {
    tracker_.reset();
}

PipelineResult Pipeline::process_frame(SceneFrame frame,
                                       const PipelineOptions& options) const {
    PipelineResult result{};
    result.frame = std::move(frame);

    const auto total_start = std::chrono::steady_clock::now();

    const auto infer_start = std::chrono::steady_clock::now();
    infer_.run(result.frame);
    const auto infer_end = std::chrono::steady_clock::now();

    if (options.enable_tracking) {
        tracker_.update(result.frame, result.frame.timestamp_seconds);
    }
    scene_graph_.update(result.frame);

    const auto predict_start = std::chrono::steady_clock::now();
    predictor_.apply(result.frame);
    const auto predict_end = std::chrono::steady_clock::now();

    if (options.enable_grok) {
        const auto grok_start = std::chrono::steady_clock::now();
        GrokRequest grok_request{};
        grok_request.scene_json = scene_graph_.to_json(result.frame);
        result.grok = grok_.narrate(grok_request);
        const auto grok_end = std::chrono::steady_clock::now();
        result.metrics.grok_ms = elapsed_ms(grok_start, grok_end);
    }

    const auto total_end = std::chrono::steady_clock::now();

    result.metrics.infer_ms = elapsed_ms(infer_start, infer_end);
    result.metrics.predict_ms = elapsed_ms(predict_start, predict_end);
    result.metrics.total_ms = elapsed_ms(total_start, total_end);

    return result;
}

PipelineResult Pipeline::process_image(const std::string& image_path,
                                       const PipelineOptions& options) const {
    const auto capture_start = std::chrono::steady_clock::now();
    SceneFrame frame = capture_.load_frame(CaptureRequest{.image_path = image_path});
    const auto capture_end = std::chrono::steady_clock::now();

    reset_tracking();
    PipelineResult result = process_frame(std::move(frame), options);
    result.metrics.capture_ms = elapsed_ms(capture_start, capture_end);
    result.metrics.total_ms += result.metrics.capture_ms;
    return result;
}

#if defined(FOVEA_HAS_VIDEO)
PipelineResult Pipeline::process_video_frame(const VideoCapture& video,
                                             int frame_index,
                                             const PipelineOptions& options) const {
    const auto capture_start = std::chrono::steady_clock::now();
    SceneFrame frame = video.read_frame_at(frame_index);
    const auto capture_end = std::chrono::steady_clock::now();

    PipelineResult result = process_frame(std::move(frame), options);
    result.metrics.capture_ms = elapsed_ms(capture_start, capture_end);
    result.metrics.total_ms += result.metrics.capture_ms;
    return result;
}
#endif

}  // namespace fovea
