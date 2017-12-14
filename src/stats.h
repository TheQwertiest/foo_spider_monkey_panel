#pragma once

namespace stats
{
	static const GUID guid_js_panel_index_playcount = { 0x2d6e21e9, 0xf079, 0x474d,{ 0xa8, 0x2d, 0xf8, 0x9a, 0x34, 0xbc, 0xf7, 0xbf } };
	static const GUID guid_js_panel_index_loved = { 0xabb2b357, 0xf21f, 0x4c08,{ 0xa1, 0xd7, 0x53, 0x76, 0xcd, 0xe7, 0xe2, 0x51 } };

	static const char strPinTo[] = "%artist% - %title%";
	static const char strPropertiesGroup[] = JSP_NAME;
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
					api->add(g_client, guid_js_panel_index_playcount, retentionPeriod);
					api->add(g_client, guid_js_panel_index_loved, retentionPeriod);
				}
				catch (std::exception const & e)
				{
					api->remove(guid_js_panel_index_playcount);
					api->remove(guid_js_panel_index_loved);
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

	static stats_t get_playcount(metadb_index_hash hash, static_api_ptr_t<metadb_index_manager> & api)
	{
		stats_t playcount;
		if (api->get_user_data_here(guid_js_panel_index_playcount, hash, &playcount, sizeof(playcount)) != sizeof(playcount)) return stats_invalid;
		return playcount;
	}

	static void set_playcount(metadb_index_hash hash, stats_t playcount)
	{
		static_api_ptr_t<metadb_index_manager> api;
		api->set_user_data(guid_js_panel_index_playcount, hash, &playcount, sizeof(playcount));
		static_api_ptr_t<metadb_index_manager>()->dispatch_refresh(guid_js_panel_index_playcount, hash);
	}

	static stats_t get_loved(metadb_index_hash hash, static_api_ptr_t<metadb_index_manager> & api)
	{
		stats_t loved;
		if (api->get_user_data_here(guid_js_panel_index_loved, hash, &loved, sizeof(loved)) != sizeof(loved)) return stats_invalid;
		return loved;
	}

	static void set_loved(metadb_index_hash hash, stats_t loved)
	{
		static_api_ptr_t<metadb_index_manager> api;
		api->set_user_data(guid_js_panel_index_loved, hash, &loved, sizeof(loved));
		static_api_ptr_t<metadb_index_manager>()->dispatch_refresh(guid_js_panel_index_loved, hash);
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
				out = "jsp_playcount";
				break;
			case 1:
				out = "jsp_loved";
				break;
			}
		}
		bool process_field(t_uint32 index, metadb_handle * handle, titleformat_text_out * out)
		{
			metadb_index_hash hash;
			if (!g_client->hashHandle(handle, hash)) return false;

			static_api_ptr_t<metadb_index_manager> api;

			switch (index)
			{
			case 0:
				{
					stats_t playcount = get_playcount(hash, api);
					if (playcount == stats_invalid) return false;
					out->write_int(titleformat_inputtypes::meta, playcount);
					return true;
				}
			case 1:
				{
					stats_t loved = get_loved(hash, api);
					if (loved == stats_invalid) return false;
					out->write_int(titleformat_inputtypes::meta, loved);
					return true;
				}
			default:
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
			pfc::avltree_t<metadb_index_hash> hashes;
			const size_t trackCount = p_tracks.get_count();
			for (size_t trackWalk = 0; trackWalk < trackCount; ++trackWalk)
			{
				metadb_index_hash hash;
				if (g_client->hashHandle(p_tracks[trackWalk], hash))
				{
					hashes += hash;
				}
			}

			static_api_ptr_t<metadb_index_manager> api;
			uint64_t total = 0;
			for (auto i = hashes.first(); i.is_valid(); ++i)
			{
				auto p = get_playcount(*i, api);
				if (p != stats_invalid)
				{
					total += p;
				}
			}

			if (total > 0)
			{
				p_out.set_property(strPropertiesGroup, 0, "Playcount", pfc::format_uint(total));
			}
		}

		void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out)
		{
			if (p_out.is_group_wanted(strPropertiesGroup))
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
