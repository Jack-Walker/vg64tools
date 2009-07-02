/*******************
* GZRT Main Window *
*******************/
#include <gzrt.h>
#include <stdarg.h>
#include <glib.h>
#include <n64rom.h>
#include <z64.h> 
#include <malloc.h>

/* Connect data to a widget */
#define HOOKUP( component, widget, name )                       \
    g_object_set_data_full( G_OBJECT(component), name, widget, NULL )

/* Lookup connected data */
#define LOOKUP( component, name )   \
    g_object_get_data( G_OBJECT(component), name )

/* Main windows */
static GList * instances;

/* Create a new window */
int gzrt_wmain_create_new ( N64Rom * rc )
{
	MAINWIN * cur = gzrt_calloc( sizeof(MAINWIN) );
	int i;
	
	/* Store the ROM context */
	cur->c = rc;
	
	/* Is this ROM already open? */
	for( i = 0; i < g_list_length(instances); i++ )
	{
		if( !strcmp((char*)rc->filename, (char*)((MAINWIN*)g_list_nth(instances, i)->data)->c->filename) )
		{
			gzrt_notice( "Notice", "This ROM is already open! ");
			n64rom_close( rc );
			gzrt_free( cur );
			return FALSE;
		}
	}
	
	/* Byteswapped? */
	if( rc->endian != N64_ENDIAN_BIG )
	{
		GtkWidget * d = gtk_dialog_new_with_buttons
		( 
			"Notice", NULL, GTK_DIALOG_MODAL, 
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			NULL 
		);
		GtkWidget * a = GTK_DIALOG(d)->vbox;
		GtkWidget * l = gtk_label_new( "This ROM needs to be byteswapped." );
		
		gtk_misc_set_padding( GTK_MISC(l), 12, 12 );
		gtk_container_add( GTK_CONTAINER(a), l );
		gtk_widget_show_all(d);
		
		switch( gtk_dialog_run( GTK_DIALOG(d) ) )
		{
			/* No */
			case GTK_RESPONSE_REJECT:	
			  n64rom_close( rc );
			  free( cur );
			  gtk_widget_destroy( d );
			  return FALSE;
			break;
			
			/* Yes */
			case GTK_RESPONSE_ACCEPT:
			  gtk_widget_destroy( d );
			  gzrt_wmain_byteswap( rc );
			break;
			
			default:
			  gtk_widget_destroy( d );
		}
	}
	
	/* Identify Zelda filesystem elements */
	if( !(cur->z = z64fs_open( (char*)rc->filename )) )
	{
		GZRTD_MESG( "Could not find filesystem!" );
		gzrt_notice( "Error", "Unable to find filesystem in ROM." );
		n64rom_close( rc );
		free( cur );
		return 0;
	}
	
	/* Identify Zelda 64 name table elements */
	if( !(cur->t = z64nt_open( rc->handle )) )
	{
		GZRTD_MESG( "No name table in this ROM." );
	}
	
	/* Information */
	GZRTD_MESG( "Name table p:  $%08X", cur->t );
	GZRTD_MESG( "File table p:  $%08X", cur->z );
	GZRTD_MESG( "ROM context p: $%08X", rc	   );
	
	/* Fill struct with GTK elements & update count */
	gzrt_wmain_fill( cur );
	
	/* Set message */
	gzrt_wmain_status_addmsg( cur, "Ready" );
	
	/* Append to list */
	instances = g_list_append( instances, cur );
	
	/* We did it! */
	return TRUE;
}

/* Byteswap a ROM */
void gzrt_wmain_byteswap ( N64Rom * rc )
{
	GtkWidget * window;
	GtkWidget * bar;
	unsigned	chunksize, i;
	char		buffer[64];
	unsigned char * rom;
	
	/* Create window */
	window = gtk_window_new( GTK_WINDOW_POPUP );
	gtk_window_set_position( GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS );
	gtk_widget_set_size_request( window, 350, 30 );
	gtk_window_set_keep_above( GTK_WINDOW(window), TRUE );
	gtk_window_set_modal( GTK_WINDOW(window), TRUE );
	
	/* Create progress bar */
	bar = gtk_progress_bar_new();
	
	/* Pack it */
	gtk_container_add( GTK_CONTAINER(window), bar );
	
	/* Show */
	gtk_widget_show_all( window );
	
	/* Set updates */
	chunksize = rc->filesize / 8;
	
	/* ROM storage */
	rom = gzrt_malloc( chunksize );
	
	/* Loop */
	for( i = 0; i < rc->filesize; i += chunksize )
	{
		/* Seek to current */
		fseek( rc->handle, i, SEEK_SET );
		
		/* Read it in */
		fread( rom, chunksize, 1, rc->handle );
		
		/* Swap it */
		if( !n64_byteswap( rom, chunksize, N64_ENDIAN_BIG, rc->endian ) )
			exit( -4 );
		
		/* Write */
		fseek( rc->handle, i, SEEK_SET );
		fwrite( rom, chunksize, 1, rc->handle );
		
		/* Update progress bar */
		sprintf( buffer, "%.2f%%", (double)i/(double)rc->filesize * 100.0 );
		gtk_progress_bar_set_text( GTK_PROGRESS_BAR(bar), buffer );
		gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(bar), (double)i/(double)rc->filesize );
		
		/* Make sure it shows up */
		while( gtk_events_pending() )
			gtk_main_iteration();
	}
	
	/* Final update */
	sprintf( buffer, "%.2f%%", (double)i/(double)rc->filesize * 100.0 );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR(bar), buffer );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(bar), (double)i/(double)rc->filesize );
	
	/* Done */
	gzrt_notice( "Notice", "ROM successfully byteswapped." );
	
	/* Delete progress bar */
	gtk_widget_destroy( window );
}

/* Close a preexisting window */
void gzrt_wmain_close ( MAINWIN *w )
{
	/* Destroy the window */
	gtk_widget_destroy( w->window );
}

/* Closed */
void gzrt_wmain_closed ( MAINWIN *w )
{
	/* Close ROM context */
	n64rom_close( w->c );
	
	/* Close filesystem context */
	z64fs_close( w->z );
	
	/* Close name table context, if applicable */
	if( w->t )
		z64nt_close( w->t );
	
	/* List of timeouts */
	gzrt_wmain_remove_timeouts(w);
	
	/* Is this the last window? If so, we can just quit */
	if( g_list_length( instances ) == 1 )
		gzrt_gui_quit();
	
	/* Remove it from the list */
	instances = g_list_remove( instances, w );
}

/* Amount of main windows */
int gzrt_wmain_count ( void )
{
	return g_list_length( instances );
}

/* Create new instance */
void gzrt_wmain_fill ( MAINWIN *c )
{
	/* GTK Elements */
	GtkWidget *Main_Window = NULL;
	GtkWidget *Main_vbox = NULL;
	GtkWidget *Menu = NULL;
	GtkWidget *File_menu = NULL;
	GtkWidget *File_menu_menu = NULL;
	GtkWidget *open1 = NULL;
	GtkWidget *reload1 = NULL;
	GtkWidget *close1 = NULL;
	GtkWidget *image5 = NULL;
	GtkWidget *quit1 = NULL;
	GtkWidget *Operations_menu = NULL;
	GtkWidget *Operations_menu_menu = NULL;
	GtkWidget *extract_files1 = NULL;
	GtkWidget *decompress_rom1 = NULL;
	GtkWidget *file_search1 = NULL;
	GtkWidget *separator1 = NULL;
	GtkWidget *byteswap_rom1 = NULL;
	GtkWidget *fix_crc1 = NULL;
	GtkWidget *Help_menu = NULL;
	GtkWidget *Help_menu_menu = NULL;
	GtkWidget *about1 = NULL;
	GtkWidget *App_status = NULL;
	GtkWidget *Frame_1_alignment = NULL;
	GtkAccelGroup *accel_group;
	
	/* Variables */
	char buffer[256];
	
	/* ~~ */
	
	
	/* Load the ROM 
	d = gzrt_wpbar_new();
	gzrt_wpbar_set( d , 0);
	gzrt_wpbar_show( d );
	if( !(n64rom_read( c->c, pbu )) )
		gzrt_werror_show( "Error occured", n64rom_error(), 1 );
	GZRTD_MESG( "Loaded ROM successfully. %08X", U32(c->c->data) );
	gzrt_wpbar_close( d );*/
	
	/* ~~ */
	
	/* Set up window */
	accel_group = gtk_accel_group_new ();
	snprintf( buffer, sizeof(buffer), "%s %s%s: %s - \"%.24s\"",
		GZRT_WMAIN_NAME, GZRT_VERSION,
	#ifdef GZRT_DEBUG
	 " *DEBUG*",
	#else
	 "",
	#endif
		c->c->filename, c->c->makerom + 0x20 );
	Main_Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (Main_Window), buffer );
	gtk_window_set_default_size (GTK_WINDOW (Main_Window), 800, 480);

	Main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (Main_vbox);
	gtk_container_add (GTK_CONTAINER (Main_Window), Main_vbox);

	Menu = gtk_menu_bar_new ();
	gtk_widget_show (Menu);
	gtk_box_pack_start (GTK_BOX (Main_vbox), Menu, FALSE, FALSE, 0);

	File_menu = gtk_menu_item_new_with_mnemonic (_("_File"));
	gtk_widget_show (File_menu);
	gtk_container_add (GTK_CONTAINER (Menu), File_menu);

	File_menu_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (File_menu), File_menu_menu);

	open1 = gzrt_gtk_image_menu_item_stock( GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU, "_Open" );
	gtk_container_add( GTK_CONTAINER(File_menu_menu), open1);
	
	reload1 = gzrt_gtk_image_menu_item_stock( GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU, "_Settings" );
	gtk_container_add( GTK_CONTAINER(File_menu_menu), reload1 );
	
	close1 = gzrt_gtk_image_menu_item_stock( GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU, "_Close" );
	gtk_container_add( GTK_CONTAINER(File_menu_menu), close1 );
	
	quit1 = gzrt_gtk_image_menu_item_stock( GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU, "_Quit" );
	gtk_container_add( GTK_CONTAINER(File_menu_menu), quit1 );

	Operations_menu = gtk_menu_item_new_with_mnemonic (_("_Operations"));
	gtk_widget_show (Operations_menu);
	gtk_container_add (GTK_CONTAINER (Menu), Operations_menu);

	Operations_menu_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (Operations_menu), Operations_menu_menu);
	
	extract_files1 = gtk_menu_item_new_with_mnemonic( "_Extract files" );
	gtk_container_add( GTK_CONTAINER(Operations_menu_menu), extract_files1 );
	
	decompress_rom1 = gtk_menu_item_new_with_mnemonic (_("_Decompress ROM"));
	gtk_widget_show (decompress_rom1);
	gtk_container_add (GTK_CONTAINER (Operations_menu_menu), decompress_rom1);

	/*
	file_search1 = gtk_menu_item_new_with_mnemonic (_("_File search"));
	gtk_widget_show (file_search1);
	gtk_container_add (GTK_CONTAINER (Operations_menu_menu), file_search1);
	*/

	separator1 = gtk_separator_menu_item_new ();
	gtk_widget_show (separator1);
	gtk_container_add (GTK_CONTAINER (Operations_menu_menu), separator1);
	gtk_widget_set_sensitive (separator1, FALSE);

	byteswap_rom1 = gtk_menu_item_new_with_mnemonic (_("_Byteswap ROM"));
	gtk_widget_show (byteswap_rom1);
	gtk_container_add (GTK_CONTAINER (Operations_menu_menu), byteswap_rom1);

	fix_crc1 = gtk_menu_item_new_with_mnemonic (_("Fix _CRC"));
	gtk_widget_show (fix_crc1);
	gtk_container_add (GTK_CONTAINER (Operations_menu_menu), fix_crc1);
	
	/* Plugins menu */
	gtk_container_add( GTK_CONTAINER(Menu), gzrt_plugins_menu() );
	

	Help_menu = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_widget_show (Help_menu);
	gtk_container_add (GTK_CONTAINER (Menu), Help_menu);

	Help_menu_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (Help_menu), Help_menu_menu);

	about1 = gzrt_gtk_image_menu_item_stock( GTK_STOCK_ABOUT, GTK_ICON_SIZE_LARGE_TOOLBAR, "_About" );
	gtk_container_add( GTK_CONTAINER(Help_menu_menu), about1 );
	
	GtkWidget * M = gzrt_wmain_main_generate(c);
	GtkWidget * MAIN_PANE = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	/*Main_window_padding = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (Main_window_padding);*/
	gtk_box_pack_start (GTK_BOX (Main_vbox), MAIN_PANE, TRUE, TRUE, 0);
	gtk_container_add( GTK_CONTAINER(MAIN_PANE), M );
	/*gtk_alignment_set_padding (GTK_ALIGNMENT (Main_window_padding), 12, 12, 12, 12);*/

	App_status = gtk_statusbar_new ();
	gtk_widget_show (App_status);
	gtk_box_pack_start (GTK_BOX (Main_vbox), App_status, FALSE, FALSE, 0);

	/* Signals - file menu */
	g_signal_connect_swapped( G_OBJECT(open1),   "activate", G_CALLBACK(gzrt_wfilesel_show), NULL );
	g_signal_connect_swapped( G_OBJECT(reload1), "activate", G_CALLBACK(gzrt_wconf_show), c );
	g_signal_connect_swapped( G_OBJECT(close1),  "activate", G_CALLBACK(gzrt_wmain_close), c );
	g_signal_connect_swapped( G_OBJECT(quit1),   "activate", G_CALLBACK(gzrt_gui_quit), NULL );
	
	//gzrt_wmain_update
	
	/* Signals - operations menu */
	g_signal_connect_swapped( G_OBJECT(extract_files1),  "activate", G_CALLBACK(gzrt_wextract_show),     c );
	g_signal_connect_swapped( G_OBJECT(decompress_rom1), "activate", G_CALLBACK(gzrt_wdecomp_show),      c );
  /*g_signal_connect_swapped( G_OBJECT(byteswap_rom1),   "activate", G_CALLBACK(gzrt_wbyteswap_create),  c ); */
	g_signal_connect_swapped( G_OBJECT(fix_crc1),        "activate", G_CALLBACK(gzrt_wcrc_show),         c );
	
	/* Signals - help menu */
	g_signal_connect_swapped( G_OBJECT(about1),"activate", G_CALLBACK(gzrt_wabout_show), c );
	
	
	/* Signals - window itself */
	g_signal_connect_swapped( G_OBJECT(Main_Window), "destroy", G_CALLBACK(gzrt_wmain_closed), c );

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF (Main_Window, Main_Window, "Main_Window");
	GLADE_HOOKUP_OBJECT (Main_Window, M, "main");
	GLADE_HOOKUP_OBJECT (Main_Window, MAIN_PANE, "mainpane");
	GLADE_HOOKUP_OBJECT (Main_Window, Main_vbox, "Main_vbox");
	GLADE_HOOKUP_OBJECT (Main_Window, Menu, "Menu");
	GLADE_HOOKUP_OBJECT (Main_Window, File_menu, "File_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, File_menu_menu, "File_menu_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, open1, "open1");
	GLADE_HOOKUP_OBJECT (Main_Window, close1, "close1");
	GLADE_HOOKUP_OBJECT (Main_Window, image5, "image5");
	GLADE_HOOKUP_OBJECT (Main_Window, quit1, "quit1");
	GLADE_HOOKUP_OBJECT (Main_Window, Operations_menu, "Operations_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, Operations_menu_menu, "Operations_menu_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, extract_files1, "extract_files1");
	GLADE_HOOKUP_OBJECT (Main_Window, decompress_rom1, "decompress_rom1");
	GLADE_HOOKUP_OBJECT (Main_Window, file_search1, "file_search1");
	GLADE_HOOKUP_OBJECT (Main_Window, separator1, "separator1");
	GLADE_HOOKUP_OBJECT (Main_Window, byteswap_rom1, "byteswap_rom1");
	GLADE_HOOKUP_OBJECT (Main_Window, fix_crc1, "fix_crc1");
	GLADE_HOOKUP_OBJECT (Main_Window, Help_menu, "Help_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, Help_menu_menu, "Help_menu_menu");
	GLADE_HOOKUP_OBJECT (Main_Window, about1, "about1");
	GLADE_HOOKUP_OBJECT (Main_Window, Frame_1_alignment, "Frame_1_alignment");
	
	GLADE_HOOKUP_OBJECT (Main_Window, App_status, "App_status");

	/* Store window pointer & show */
	c->window = Main_Window;
	gtk_widget_show_all( Main_Window );
	
	gtk_window_add_accel_group (GTK_WINDOW (Main_Window), accel_group);
}
	
	#define MONO(x) "<span font_desc=\"Courier\">", x, "</span>"

/* Update the display in the main window */
void gzrt_wmain_update ( MAINWIN *c )
{
	/*
	** This function will reload the opened ROM and in turn
	** update all of the controls in the main window.
	*/
	
	GtkWidget * m = g_object_get_data( G_OBJECT(c->window), "main" );
	GtkWidget * h = g_object_get_data( G_OBJECT(c->window), "mainpane" );
	
	gtk_widget_destroy(m);
	
	m = gzrt_wmain_main_generate(c);
	
	gtk_container_add( GTK_CONTAINER(h), m );
	
	gtk_widget_show_all( h );
	
	GLADE_HOOKUP_OBJECT( c->window, m, "main" );
}

/* Add a new element to a side frame */
void gzrt_wmain_frame_add ( GtkWidget *vbox, char *fmt, ... )
{
	char buffer[256];
	GtkWidget * label;
	va_list ap;
	
	/* Generate text */
	va_start( ap, fmt );
	vsnprintf( buffer, sizeof(buffer) - 1, fmt, ap );
	va_end( ap );
	
	/* Create label */
	label = gtk_label_new( "" );
	gtk_label_set_markup( GTK_LABEL(label), buffer );
	
	/* Set alignment prefs */
	gtk_misc_set_alignment( GTK_MISC(label), 0.0f, 0.5f );
	
	/* Show */
	gtk_widget_show( label );
	
	/* Pack it */
	gtk_box_pack_start( GTK_BOX(vbox), label, TRUE, TRUE, 0 );
}

/* Get object from main window */
static GtkWidget * get_object ( MAINWIN * w, char * name )
{
	GtkWidget * o = g_object_get_data( G_OBJECT(w->window), "main" );
	
	return g_object_get_data( G_OBJECT(o), name );
}

/* Plugin action */
void gzrt_wmain_plugin_action ( MAINWIN * w )
{
	struct PluginFileSpec * file = gzrt_calloc( sizeof(struct PluginFileSpec) );
	int	id =  gzrt_select_file_id(w);
	const Z64FSEntry * j = z64fs_file( w->z, id );
	
	/* Check */
	if( !ZFileExists(w->z, id) )
	{
		gzrt_notice("Notice", "This file does not physically exist within the ROM.\n"
		"No operation can be performed on it." );
		gzrt_free( file );
		return;
	}
	
	/* Fill the file information struct */
	file->id		= id;
	file->vstart	= j->vstart;
	file->vend		= j->vend;
	file->start		= j->start;
	file->end		= j->end;
	file->filesize  = ZFileVirtSize(w->z,id);
	
	/* Write filename */
	if( w->t )
		strncpy( file->filename, z64nt_filename(w->t, id), sizeof(file->filename) - 1 );
	else
		snprintf( file->filename, sizeof(file->filename), "0x%08X - 0x%08X",
		j->vstart, j->vend );
	
	/* Read the file */
	file->file = gzrt_malloc( file->filesize );
	z64fs_read_file( w->z, id, file->file );
	
	gzrt_call_plugin( &w->spec, file );
}

/* Get ID of the currently selected file */
int	gzrt_select_file_id ( MAINWIN * w )
{
	
    GList		     * list		 = NULL;
	GList		     * llist 	 = NULL;
	GtkWidget        * tv 		 = get_object( w, "file-tree" );
	GtkTreeModel*      model	 = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
	GtkTreeSelection * selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    GtkTreeIter	       iter;
	char             * s;
	
	/* No selection? */
	if( !selection )
		return -1;
	
	/* - */
	if( !(list = gtk_tree_selection_get_selected_rows (selection, &model)) )
		return -1;
    llist = g_list_first(list);
    gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)llist->data);
    gtk_tree_model_get(model, &iter, 0, &s, -1);
    g_list_free(list);
	
	/* Return file ID */
	return atoi( s );
}
	

/* Disable an item */
void gzrt_wmain_disable_item ( MAINWIN *w, char *name )
{
	GtkWidget *i = g_object_get_data( G_OBJECT(w->window), name );
	gtk_widget_set_sensitive( i, FALSE );
}

/* Enable an item */
void gzrt_wmain_enable_item ( MAINWIN *w, char *name )
{
	GtkWidget *i = g_object_get_data( G_OBJECT(w->window), name );
	gtk_widget_set_sensitive( i, TRUE );
}

/* Set the status bar */
void gzrt_wmain_status_addmsg ( MAINWIN *w, char *msg )
{
	GtkWidget *i = g_object_get_data( G_OBJECT(w->window), "App_status" );
	gtk_statusbar_push( GTK_STATUSBAR(i), w->sstack++, msg );
	GZRTD_MESG( "Status message changed to: %s", msg );
}

/* Remove a message */
void gzrt_wmain_status_rmmsg ( MAINWIN *w )
{
	GtkWidget *i = g_object_get_data( G_OBJECT(w->window), "App_status" );
	gtk_statusbar_pop( GTK_STATUSBAR(i), --w->sstack );
	GZRTD_MESG( "Status reverted." );
}

/* Create a markup'd label */
static GtkWidget * create_label ( char * fmt, ... )
{
	GtkWidget * label;
	va_list		ap;
	char		buffer[512];
	int			len;
	
	/* Prepare string */
	va_start( ap, fmt );
	len = vsnprintf( buffer, sizeof(buffer), fmt, ap );
	va_end( ap );
	
	/* Create widget */
	label = gtk_label_new( buffer );
	gtk_misc_set_alignment( GTK_MISC(label), 0.0f, 0.5f );
	gtk_label_set_use_markup( GTK_LABEL(label), TRUE );
	gtk_widget_show( label );
	
	/* Return it */
	return label;
}
	
/* Create ROM information frame */
static GtkWidget * create_rom_info_frame ( MAINWIN * c )
{
	GtkWidget * frame;
	GtkWidget * align;
	GtkWidget * vbox;
	GtkWidget * name;
	
	/* Create frame */
	frame = gtk_frame_new( NULL );
	
	/* Set name */
	name = gtk_label_new( "<b>ROM info</b>" );
	gtk_label_set_use_markup( GTK_LABEL(name), TRUE );
	gtk_frame_set_label_widget( GTK_FRAME(frame), name );
	
	/* Create label alignment */
	align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(align), 8, 8, 12, 12 );
	gtk_container_add( GTK_CONTAINER(frame), align );
	
	/* Create labels box */
	vbox = gtk_vbox_new( FALSE, 8 );
	gtk_container_add( GTK_CONTAINER(align), vbox );
	
	/* Pack info */
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Name: %.24s", c->c->makerom + 0x20), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Code: %.4s", c->c->makerom + 0x3B), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Size: %.2fMB (%.2fMBits)", (float)c->c->filesize / 1024.0 / 1024.0, (float)c->c->filesize / 1024.0 / 1024.0 * 8.0), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("CRC 1: <span font_desc=\"Courier\">0x%08X</span>", U32(c->c->makerom + 0x10)), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("CRC 2: <span font_desc=\"Courier\">0x%08X</span>", U32(c->c->makerom + 0x14)), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Entry point: <span font_desc=\"Courier\">0x%08X</span>", U32(c->c->makerom + 0x08)), 
	FALSE, TRUE, 0 );
	
	return frame;
}

/* Create filesystem information frame */
static GtkWidget * create_bin_info_frame ( MAINWIN * c )
{
	GtkWidget * frame;
	GtkWidget * align;
	GtkWidget * vbox;
	GtkWidget * name;
	
	/* Create frame */
	frame = gtk_frame_new( NULL );
	
	/* Set name */
	name = gtk_label_new( "<b>Filesystem info</b>" );
	gtk_label_set_use_markup( GTK_LABEL(name), TRUE );
	gtk_frame_set_label_widget( GTK_FRAME(frame), name );
	
	/* Create label alignment */
	align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(align), 8, 8, 12, 12 );
	gtk_container_add( GTK_CONTAINER(frame), align );
	
	/* Create labels box */
	vbox = gtk_vbox_new( FALSE, 8 );
	gtk_container_add( GTK_CONTAINER(align), vbox );
	
	/* Pack info */
	#define MONO(x) "<span font_desc=\"Courier\">", x, "</span>"
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Filesystem start: %s0x%08X%s", MONO(c->z->start)), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Filesystem end: %s0x%08X%s", MONO(c->z->end)), 
	FALSE, TRUE, 0 );
	
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("File count: %u", z64fs_entries( c->z )), 
	FALSE, TRUE, 0 );
	/*
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Total size: %.2fMB", (float)z64fs_calc_size_decompressed( c->z ) / 1024.0 / 1024.0), 
	TRUE, TRUE, 0 );
	*/
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Creator: %s%s%s", MONO(c->z->creator) ), 
	FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), 
		create_label("Date: %s%s%s", MONO(c->z->date) ), 
	FALSE, TRUE, 0 );
	
	/* Name table? */
	if( c->t )
	{
		gtk_box_pack_start( GTK_BOX(vbox), 
			create_label("Name table start: %s0x%08X%s", MONO(z64nt_start(c->t))), 
		FALSE, TRUE, 0 );
		gtk_box_pack_start( GTK_BOX(vbox), 
			create_label("Name table end: %s0x%08X%s", MONO(z64nt_end(c->t))), 
		FALSE, TRUE, 0 );
	}
	else
	{
		gtk_box_pack_start( GTK_BOX(vbox), 
			create_label("Name table: %s%s%s", MONO("no")), 
		FALSE, TRUE, 0 );
	}
	
	return frame;
}

/* Populate the main window */
GtkWidget * gzrt_wmain_main_generate ( MAINWIN * w )
{
	/* GTK Elements */
	GtkWidget * ret;					/* An alignment containing everything */
	
	GtkWidget * content_hbox;
	GtkWidget * info_list_separator;	/* Separates info box and file list   */
  /*GtkWidget * info_pane;				   Resizable pane for information	  */
	GtkWidget * info_vbox;				/* Information storage box (frames)   */
	GtkWidget * info_rom_align;			/* Align the frame nicely			  */
	GtkWidget * info_bin_align;			/* Align the frame nicely			  */
	GtkWidget * flist_align;			/*									  */
	GtkWidget * flist_vbox;				/* Tree view and buttons sep		  */
	GtkWidget * flist_scroll;			/* Scrolled window for tree			  */
	GtkWidget * flist_tree;				/* The tree containing file listing	  */
	GtkWidget * flist_button_hbox;		/* For the action buttons			  */
	
	/* Functions */
	extern GtkWidget * gzrt_wmain_tree_generate ( MAINWIN * c );
	
	/* Create the alignemnt */
	ret = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(ret), 12, 12, 12, 12 );
	
	/* Create the horizontal box separating info pane from file list */
	info_list_separator = gtk_hbox_new( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER(ret), info_list_separator );
	
	/* Create the draggable pane */
	/* info_pane = gtk_hpaned_new(); */
	content_hbox = gtk_hbox_new( FALSE, 8 );
	gtk_box_pack_start( GTK_BOX(info_list_separator), content_hbox, TRUE, TRUE, 0 );
	
	/* Create the vertical box for all the frames */
	info_vbox = gtk_vbox_new( FALSE, 0 );
	gtk_box_pack_start( GTK_BOX(content_hbox), info_vbox, FALSE, TRUE, 0 );
	
	/* Create alignments for frames */
	info_rom_align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(info_rom_align), 0, 4, 0, 4 );
	info_bin_align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(info_bin_align), 4, 0, 0, 4 );
	
	/* Pack them in */
	gtk_box_pack_start( GTK_BOX(info_vbox), info_rom_align, TRUE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(info_vbox), info_bin_align, TRUE, TRUE, 0 );
	
	/* Create the frames */
	gtk_container_add( GTK_CONTAINER(info_rom_align), create_rom_info_frame(w) );
	gtk_container_add( GTK_CONTAINER(info_bin_align), create_bin_info_frame(w) );
	
	
	/* ALignment */
	flist_align = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	gtk_alignment_set_padding( GTK_ALIGNMENT(flist_align), 0, 0, 4, 0 );
	gtk_box_pack_start( GTK_BOX(content_hbox), flist_align, TRUE, TRUE, 0 );
	
	/* Create right vbox */
	flist_vbox = gtk_vbox_new( FALSE, 4 );
	gtk_container_add( GTK_CONTAINER(flist_align), flist_vbox );
	
	/* Create file listing */
	GtkWidget * note = gtk_notebook_new();
	flist_scroll = gtk_scrolled_window_new( NULL, NULL );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(flist_scroll), GTK_SHADOW_IN );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(flist_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	flist_tree = gzrt_wmain_tree_generate(w);
	gtk_container_add( GTK_CONTAINER(flist_scroll), flist_tree );
	gtk_notebook_append_page( GTK_NOTEBOOK(note), flist_scroll, 
		gzrt_gtk_image_label( GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_LARGE_TOOLBAR, "Flat view" )
	);
	gtk_notebook_append_page( GTK_NOTEBOOK(note), gtk_label_new( "Soon" ),  
		gzrt_gtk_image_label( GTK_STOCK_SELECT_COLOR, GTK_ICON_SIZE_LARGE_TOOLBAR, "Filetype tree" ) );
	gtk_notebook_append_page( GTK_NOTEBOOK(note), gtk_label_new( "Soon" ),  
		gzrt_gtk_image_label( GTK_STOCK_INFO, GTK_ICON_SIZE_LARGE_TOOLBAR, "File info" ) );
	gtk_box_pack_start( GTK_BOX(flist_vbox), note, TRUE, TRUE, 0 );
	
	/* Debug tab */ {
	extern int gzrt_wmain_mem_use ( GtkWidget * label );
	
	GtkWidget * align  = gtk_alignment_new( 0.5f, 0.5f, 1.0f, 1.0f );
	GtkWidget * vbox   = gtk_vbox_new( FALSE, 8 );
	GtkWidget * hbox   = gtk_hbox_new( FALSE, 12 );
	GtkWidget * text   = gtk_text_view_new_with_buffer
	( 
		(!debug_console ? debug_console = gtk_text_buffer_new(NULL) : debug_console) 
	);
	GtkWidget * scroll = gtk_scrolled_window_new( NULL, NULL );
	GtkWidget * label  = gtk_label_new( "<b>Memory usage:</b>" );
	GtkWidget * pbar = gtk_progress_bar_new();
	gtk_box_pack_start( GTK_BOX(hbox), vbox, TRUE, TRUE, 0 );
	gtk_label_set_use_markup( GTK_LABEL(label), TRUE );
	gtk_misc_set_alignment( GTK_MISC(label), 0.0f, 0.0f );
	
	gtk_alignment_set_padding( GTK_ALIGNMENT(align), 8, 8, 8, 8 );
	gtk_container_add( GTK_CONTAINER(align), hbox );
	gtk_box_pack_start( GTK_BOX(vbox), label, FALSE, FALSE, 0 );
	gtk_box_pack_start( GTK_BOX(vbox), scroll, TRUE, TRUE, 0 );
	
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_container_add( GTK_CONTAINER(scroll), text );
	gtk_notebook_append_page( GTK_NOTEBOOK(note), align, 
		gzrt_gtk_image_label( GTK_STOCK_EXECUTE, GTK_ICON_SIZE_LARGE_TOOLBAR, "Console" ) );
	gzrt_gtk_font_set( text, "Courier 10" );
	
	/* Progress bar */
	gtk_progress_bar_set_orientation( GTK_PROGRESS_BAR(pbar), GTK_PROGRESS_TOP_TO_BOTTOM );
	gtk_progress_bar_set_pulse_step( GTK_PROGRESS_BAR(pbar), 0.01 );
	
	/* extern gboolean pulse_bar ( GtkWidget * b );
	gtk_box_pack_start( GTK_BOX(hbox), pbar, FALSE, TRUE, 0 );
	gtk_timeout_add( 100, (void*)pulse_bar, pbar ); */
	gzrt_wmain_timeout_add( w, 500, (void*)gzrt_wmain_mem_use, label );
}
	
	/* Set up the buttons */
	flist_button_hbox = gtk_hbox_new( TRUE, 0 );
	gtk_box_pack_start( GTK_BOX(flist_vbox), flist_button_hbox, FALSE, TRUE, 0 );
	
	/* Hookup objects */
	HOOKUP( ret, flist_tree, "file-tree" );
	
	/* Return the final product */
	return ret;
}

gboolean pulse_bar ( GtkWidget * b )
{
	gtk_progress_bar_pulse( GTK_PROGRESS_BAR(b) );
	return TRUE;
}

/* Set the label on the plugin button */
void gzrt_wmain_set_plugin ( char * n )
{
	int i, l = g_list_length( instances );
	
	for( i = 0; i < l; i++ )
	{
		MAINWIN * cur = g_list_nth( instances, i )->data;
		
		gtk_label_set_text( GTK_LABEL(cur->plugin_label), n );
	}
}

/* Extract a file */
void gzrt_wmain_extract ( MAINWIN * w )
{
	int	id = gzrt_select_file_id(w);
	GtkWidget * dialog;
	int result;
	FILE * h;

	/* Create file saving dialog */
	dialog = gtk_file_chooser_dialog_new
	( 
		"Choose a destination", NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE, 
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT, 
		NULL
	);
	gtk_widget_show_all( dialog );
	
	/* Run the dialog and fetch the result */
	while( (result = gtk_dialog_run( GTK_DIALOG(dialog) )) )
	switch( result )
	{
		/* A file has been chosen */
		case GTK_RESPONSE_ACCEPT:
		{
			char * n = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(dialog) );
			
			/* Check that file doesn't exist */
			if( (h = fopen(n, "rb")) )
			{
				gzrt_notice("Error", "File already exists.");
				fclose( h );
				continue;
			}
				
			/* Check that we can write to it */
			if( !(h = fopen(n, "wb")) )
			{
				gzrt_notice("Error", "Can't write to that file!");
				continue;
			}
			
			/* Destroy window */
			gtk_widget_destroy( dialog );
			
			/* Looks good, gogogo */
			goto write_file;
		}
		break;
		
		/* Cancel */
		case GTK_RESPONSE_CANCEL:
		 gtk_widget_destroy(dialog);
		 return;
		break;
		
		/* Default */
		default:
		 return;
	}
	
	return;
	
	/* Write file */
write_file: ;
	
	unsigned char * buffer = malloc(ZFileVirtSize(w->z, id));
	
	/* Read it */
	z64fs_read_file( w->z, id, buffer );
	
	/* Write it */
	fwrite( buffer, 1, ZFileVirtSize(w->z, id), h );
	fclose( h );
	
	/* Done */
	gtk_message_dialog_new( GTK_WINDOW(w->window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "File written successfully." );
}


/* Update memory usage */
int gzrt_wmain_mem_use ( GtkWidget * label )
{
	char buffer[128];
	
	if( !label ) 
		return FALSE;
	
	#ifndef WIN32
	 struct mallinfo info = mallinfo();
	 snprintf( buffer, sizeof(buffer), "<b>Memory usage:</b> %gmb", info.uordblks / 1024.0 / 1024.0 );
	#else
	 snprintf( buffer, sizeof(buffer), "<b>Memory usage:</b> %u bytes", gzrt_mem_use() );
	#endif
	gtk_label_set_text( GTK_LABEL(label), buffer );
	gtk_label_set_use_markup( GTK_LABEL(label), TRUE );
	
	return TRUE;
}

/* Add a timeout */
void gzrt_wmain_timeout_add ( MAINWIN * c, guint32 interval, void * func, void * object )
{
	gint id = gtk_timeout_add( interval, func, object );
	c->timeouts = g_list_append( c->timeouts, (gpointer)id );
}

/* Remove all timeouts */
void gzrt_wmain_remove_timeouts ( MAINWIN * c )
{
	gint count = g_list_length( c->timeouts ), i;
	
	for( i = 0; i < count; i++ )
	{
		GList * cur = g_list_nth( c->timeouts, i );
		
		gtk_timeout_remove( (guint)cur->data );
	}
	
	g_list_free( c->timeouts );
}

/* Copy file offsets to clipboard */
void gzrt_wmain_voffset_clipboard ( MAINWIN * c )
{
	GtkClipboard * clip;
	char buffer[64];
	int len, id = gzrt_select_file_id(c);
	
	clip = gtk_clipboard_get_for_display( gdk_display_get_default(), GDK_SELECTION_CLIPBOARD );
	len = snprintf( buffer, sizeof(buffer), "%08X %08X", ZFileVirtStart(c->z, id), ZFileVirtEnd(c->z, id) );
	gtk_clipboard_set_text( GTK_CLIPBOARD(clip), buffer, len );
	
	gtk_clipboard_store( clip );
}

/* Copy file offsets to clipboard */
void gzrt_wmain_roffset_clipboard ( MAINWIN * c )
{
	GtkClipboard * clip;
	char buffer[64];
	int len, id = gzrt_select_file_id(c);
	
	clip = gtk_clipboard_get_for_display( gdk_display_get_default(), GDK_SELECTION_CLIPBOARD );
	len = snprintf( buffer, sizeof(buffer), "%08X %08X", ZFileRealStart(c->z, id), ZFileRealEnd(c->z, id) );
	gtk_clipboard_set_text( clip, buffer, len );
	
	gtk_clipboard_store( clip );
}
