#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "Emu/Audio/FAudio/faudio_enumerator.h"
#include <locale>
#include <codecvt>
#include <array>
#include "util/logs.hpp"

LOG_CHANNEL(faudio_dev_enum);

faudio_enumerator::faudio_enumerator() : audio_device_enumerator()
{
	FAudio *tmp{};

	if (u32 res = FAudioCreate(&tmp, 0, FAUDIO_DEFAULT_PROCESSOR))
	{
		faudio_dev_enum.error("FAudioCreate() failed(0x%08x)", res);
		return;
	}

	// All succeeded, "commit"
	instance = tmp;
}

faudio_enumerator::~faudio_enumerator()
{
	if (instance != nullptr)
	{
		FAudio_StopEngine(instance);
		FAudio_Release(instance);
	}
}

std::vector<audio_device_enumerator::audio_device> faudio_enumerator::get_output_devices()
{
	if (!instance)
	{
		return {};
	}

	u32 dev_cnt{};
	if (u32 res = FAudio_GetDeviceCount(instance, &dev_cnt))
	{
		faudio_dev_enum.error("FAudio_GetDeviceCount() failed(0x%08x)", res);
		return {};
	}

	if (dev_cnt == 0)
	{
		faudio_dev_enum.warning("No devices available");
		return {};
	}

	std::vector<audio_device> device_list{};

	for (u32 dev_idx = 0; dev_idx < dev_cnt; dev_idx++)
	{
		FAudioDeviceDetails dev_info{};

		if (u32 res = FAudio_GetDeviceDetails(instance, dev_idx, &dev_info))
		{
			faudio_dev_enum.error("FAudio_GetDeviceDetails() failed(0x%08x)", res);
			continue;
		}

		const std::string dev_name = [&]() -> std::string
		{
			constexpr usz dev_name_size = sizeof(dev_info.DisplayName) / sizeof(dev_info.DisplayName[0]);
			std::array<char16_t, dev_name_size + 1> temp_buf;
			memcpy(temp_buf.data(), dev_info.DisplayName, dev_name_size);
			temp_buf[dev_name_size] = 0;
			if (std::char_traits<char16_t>::length(temp_buf.data()) == 0)
			{
				return {};
			}

			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cnv{};
			return cnv.to_bytes(temp_buf.data());
		}();

		audio_device dev =
		{
			.id = std::to_string(dev_idx),
			.name = dev_name,
			.max_ch = dev_info.OutputFormat.Format.nChannel
		};

		if (dev.name.empty())
		{
			dev.name = "Device " + dev.id;
		}

		device_list.emplace_back(dev);
	}

	return device_list;
}
