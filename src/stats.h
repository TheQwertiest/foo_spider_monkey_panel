#pragma once

namespace stats
{
	static const GUID guid_js_panel_index = { 0x835f0b63, 0xd96c, 0x447b,{ 0x9c, 0xcb, 0x71, 0x4f, 0xa8, 0x30, 0x49, 0x11 } };
	static const char strPinTo[] = "%artist% - %title%";
	static const t_filetimestamp retentionPeriod = system_time_periods::week * 4;

	static titleformat_object::ptr makeKeyObj(const char * pinTo)
	{
		titleformat_object::ptr obj;
		static_api_ptr_t<titleformat_compiler>()->compile_force(obj, pinTo);
		return obj;
	}

	class metadb_index_client_impl : public metadb_index_client
	{
	public:
		metadb_index_hash transform(const file_info & info, const playable_location & location)
		{
			static auto obj = makeKeyObj(strPinTo);
			pfc::string_formatter str;
			obj->run_simple(location, &info, str);
			return static_api_ptr_t<hasher_md5>()->process_single_string(str).xorHalve();
		}
	};
	static metadb_index_client_impl * g_client = new service_impl_single_t<metadb_index_client_impl>;

	class init_stage_callback_impl : public init_stage_callback
	{
	public:
		void on_init_stage(t_uint32 stage)
		{
			if (stage == init_stages::before_config_read)
			{
				static_api_ptr_t<metadb_index_manager> api;
				try
				{
					api->add(g_client, guid_js_panel_index, retentionPeriod);
				}
				catch (std::exception const & e)
				{
					api->remove(guid_js_panel_index);
					FB2K_console_formatter() << JSP_NAME << " stats: Critical initialisation failure: " << e;
					return;
				}
				api->dispatch_global_refresh();
			}
		}
	};
	static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;

	typedef uint32_t stats_t;
	static const stats_t stats_invalid = 0;
	struct fields
	{
		stats_t playcount;
		stats_t loved;
	};

	static stats_t get_loved(metadb_index_hash hash, static_api_ptr_t<metadb_index_manager> & api)
	{
		fields tmp;
		return api->get_user_data_here(guid_js_panel_index, hash, &tmp, sizeof(tmp)) == sizeof(tmp) ? tmp.loved : stats_invalid;
	}

	static stats_t get_playcount(metadb_index_hash hash, static_api_ptr_t<metadb_index_manager> & api)
	{
		fields tmp;
		return api->get_user_data_here(guid_js_panel_index, hash, &tmp, sizeof(tmp)) == sizeof(tmp) ? tmp.playcount : stats_invalid;
	}

	static void set_loved(metadb_index_hash hash, stats_t loved)
	{
		static_api_ptr_t<metadb_index_manager> api;
		fields tmp;
		tmp.loved = loved;
		tmp.playcount = get_playcount(hash, api);
		api->set_user_data(guid_js_panel_index, hash, &tmp, sizeof(tmp));
		api->dispatch_refresh(guid_js_panel_index, hash);
	}

	static void set_playcount(metadb_index_hash hash, stats_t playcount)
	{
		static_api_ptr_t<metadb_index_manager> api;
		fields tmp;
		tmp.loved = get_loved(hash, api);
		tmp.playcount = playcount;
		api->set_user_data(guid_js_panel_index, hash, &tmp, sizeof(tmp));
		api->dispatch_refresh(guid_js_panel_index, hash);
	}

	class metadb_display_field_provider_impl : public metadb_display_field_provider
	{
	public:
		t_uint32 get_field_count()
		{
			return 2;
		}
		void get_field_name(t_uint32 index, pfc::string_base & out)
		{
			switch (index)
			{
			case 0:
				out = "jsp_loved";
				break;
			case 1:
				out = "jsp_playcount";
				break;
			}
		}
		bool process_field(t_uint32 index, metadb_handle * handle, titleformat_text_out * out)
		{
			metadb_index_hash hash;
			if (!g_client->hashHandle(handle, hash)) return false;

			static_api_ptr_t<metadb_index_manager> api;

			if (index < get_field_count())
			{
				stats_t value = index == 0 ? get_loved(hash, api) : get_playcount(hash, api);
				if (value == stats_invalid) return false;
				out->write_int(titleformat_inputtypes::meta, value);
				return true;
			}
			else
			{
				return false;
			}
		}
	};
	static service_factory_single_t<metadb_display_field_provider_impl> g_metadb_display_field_provider_impl;

	class track_property_provider_impl : public track_property_provider_v2
	{
	public:
		void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out)
		{
			static_api_ptr_t<metadb_index_manager> api;
			const size_t trackCount = p_tracks.get_count();

			if (trackCount == 1)
			{
				metadb_index_hash hash;
				if (g_client->hashHandle(p_tracks[0], hash))
				{
					p_out.set_property(JSP_NAME, 0, "Playcount", pfc::format_uint(get_playcount(hash, api)));
					p_out.set_property(JSP_NAME, 1, "Loved", get_loved(hash, api) == stats_invalid ? "No" : "Yes");
				}
			}
			else
			{
				pfc::avltree_t<metadb_index_hash> hashes;

				for (size_t trackWalk = 0; trackWalk < trackCount; ++trackWalk)
				{
					metadb_index_hash hash;
					if (g_client->hashHandle(p_tracks[trackWalk], hash))
					{
						hashes += hash;
					}
				}

				uint64_t total = 0;
				for (auto i = hashes.first(); i.is_valid(); ++i)
				{
					total += get_playcount(*i, api);
				}

				if (total > 0)
				{
					p_out.set_property(JSP_NAME, 0, "Playcount", pfc::format_uint(total));
				}
			}
		}

		void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out)
		{
			if (p_out.is_group_wanted(JSP_NAME))
			{
				enumerate_properties(p_tracks, p_out);
			}
		}

		bool is_our_tech_info(const char * p_name)
		{
			return false;
		}

	};

	static service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;
}
