#pragma once

namespace stats
{
	static const GUID guid_js_panel_index = { 0x835f0b63, 0xd96c, 0x447b,{ 0x9c, 0xcb, 0x71, 0x4f, 0xa8, 0x30, 0x49, 0x11 } };
	static const char pinTo[] = "$lower(%artist% - %title%)";
	static const t_filetimestamp retentionPeriod = system_time_periods::week * 4;

	class metadb_index_client_impl : public metadb_index_client
	{
	public:
		metadb_index_hash transform(const file_info& info, const playable_location& location)
		{
			titleformat_object::ptr obj;
			titleformat_compiler::get()->compile_force(obj, pinTo);
			pfc::string_formatter str;
			obj->run_simple(location, &info, str);
			return hasher_md5::get()->process_single_string(str).xorHalve();
		}
	};
	static metadb_index_client_impl* g_client = new service_impl_single_t<metadb_index_client_impl>;

	static metadb_index_manager::ptr g_cachedAPI;
	static metadb_index_manager::ptr theAPI()
	{
		auto ret = g_cachedAPI;
		if (ret.is_empty()) ret = metadb_index_manager::get();
		return ret;
	}

	class init_stage_callback_impl : public init_stage_callback
	{
	public:
		void on_init_stage(t_uint32 stage)
		{
			if (stage == init_stages::before_config_read)
			{
				auto api = metadb_index_manager::get();
				g_cachedAPI = api;
				try
				{
					api->add(g_client, guid_js_panel_index, retentionPeriod);
				}
				catch (std::exception const& e)
				{
					api->remove(guid_js_panel_index);
					FB2K_console_formatter() << JSP_NAME " stats: Critical initialisation failure: " << e;
					return;
				}
				api->dispatch_global_refresh();
			}
		}
	};
	static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;

	class initquit_impl : public initquit
	{
	public:
		void on_quit()
		{
			g_cachedAPI.release();
		}
	};
	static service_factory_single_t<initquit_impl> g_initquit_impl;

	typedef uint32_t stats_t;
	static const stats_t stats_invalid = 0;
	struct fields
	{
		stats_t playcount = stats_invalid;
		stats_t loved = stats_invalid;
		pfc::string8 first_played;
		pfc::string8 last_played;
		stats_t rating = stats_invalid;
	};

	static fields get(metadb_index_hash hash)
	{
		mem_block_container_impl temp;
		theAPI()->get_user_data(guid_js_panel_index, hash, temp);
		if (temp.get_size() > 0)
		{
			try
			{
				stream_reader_formatter_simple_ref<false> reader(temp.get_ptr(), temp.get_size());
				fields ret;
				reader >> ret.playcount;
				reader >> ret.loved;
				reader >> ret.first_played;
				reader >> ret.last_played;
				if (reader.get_remaining() > 0) // check needed here for compatibility with v2 Beta4
				{
					reader >> ret.rating;
				}
				return ret;
			}
			catch (exception_io_data)
			{
			}
		}
		return fields();
	}

	static void set(metadb_index_hash hash, fields f)
	{
		stream_writer_formatter_simple<false> writer;
		writer << f.playcount;
		writer << f.loved;
		writer << f.first_played;
		writer << f.last_played;
		writer << f.rating;
		theAPI()->set_user_data(guid_js_panel_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size());
	}

	class metadb_display_field_provider_impl : public metadb_display_field_provider
	{
	public:
		t_uint32 get_field_count()
		{
			return 5;
		}
		void get_field_name(t_uint32 index, pfc::string_base& out)
		{
			switch (index)
			{
			case 0:
				out = "jsp_playcount";
				break;
			case 1:
				out = "jsp_loved";
				break;
			case 2:
				out = "jsp_first_played";
				break;
			case 3:
				out = "jsp_last_played";
				break;
			case 4:
				out = "jsp_rating";
				break;
			}
		}
		bool process_field(t_uint32 index, metadb_handle* handle, titleformat_text_out* out)
		{
			metadb_index_hash hash;
			if (!g_client->hashHandle(handle, hash)) return false;
			fields tmp = get(hash);

			switch (index)
			{
			case 0:
				{
					stats_t value = tmp.playcount;
					if (value == stats_invalid) return false;
					out->write_int(titleformat_inputtypes::meta, value);
					return true;
				}
			case 1:
				{
					stats_t value = tmp.loved;
					if (value == stats_invalid) return false;
					out->write_int(titleformat_inputtypes::meta, value);
					return true;
				}
			case 2:
				{
					pfc::string8 value = tmp.first_played;
					if (value.is_empty()) return false;
					out->write(titleformat_inputtypes::meta, value);
					return true;
				}
			case 3:
				{
					pfc::string8 value = tmp.last_played;
					if (value.is_empty()) return false;
					out->write(titleformat_inputtypes::meta, value);
					return true;
				}
			case 4:
				{
					stats_t value = tmp.rating;
					if (value == stats_invalid) return false;
					out->write_int(titleformat_inputtypes::meta, value);
					return true;
				}
			}
			return false;
		}
	};
	static service_factory_single_t<metadb_display_field_provider_impl> g_metadb_display_field_provider_impl;

	class track_property_provider_impl : public track_property_provider_v2
	{
	public:
		void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback& p_out)
		{
			const t_size trackCount = p_tracks.get_count();
			if (trackCount == 1)
			{
				metadb_index_hash hash;
				if (g_client->hashHandle(p_tracks[0], hash))
				{
					fields tmp = get(hash);
					p_out.set_property(JSP_NAME, 0, "Playcount", pfc::format_uint(tmp.playcount));
					p_out.set_property(JSP_NAME, 1, "Loved", pfc::format_uint(tmp.loved));
					p_out.set_property(JSP_NAME, 2, "First Played", tmp.first_played);
					p_out.set_property(JSP_NAME, 3, "Last Played", tmp.last_played);
					p_out.set_property(JSP_NAME, 4, "Rating", pfc::format_uint(tmp.rating));
				}
			}
			else
			{
				pfc::avltree_t<metadb_index_hash> hashes;

				for (t_size trackWalk = 0; trackWalk < trackCount; ++trackWalk)
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
					total += get(*i).playcount;
				}

				if (total > 0)
				{
					p_out.set_property(JSP_NAME, 0, "Playcount", pfc::format_uint(total));
				}
			}
		}

		void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2& p_out)
		{
			if (p_out.is_group_wanted(JSP_NAME))
			{
				enumerate_properties(p_tracks, p_out);
			}
		}

		bool is_our_tech_info(const char* p_name)
		{
			return false;
		}

	};
	static service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;
}
