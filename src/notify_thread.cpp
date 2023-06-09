#include "config.h"
#include "bulk_extractor.h"
#include "notify_thread.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#ifdef HAVE_TERM_H
#include <term.h>
#endif

int notify_thread::terminal_width( int default_width )
{
#if defined(HAVE_IOCTL) && defined(HAVE_STRUCT_WINSIZE_WS_COL)
    struct winsize ws;
    if ( ioctl( STDIN_FILENO, TIOCGWINSZ, &ws)==0){
        return  ws.ws_col;
    }
#endif
    return default_width;
}


notify_thread::~notify_thread()
{
    join();
}

void *notify_thread::run()
{
    const char *cl="";
    const char *ho="";
    const char *ce="";
    const char *cd="";
    int cols = 80;
#ifdef HAVE_LIBTERMCAP
    char buf[65536], *table=buf;
    cols = tgetnum( const_cast<char *>("co") );
    if ( !cfg.opt_legacy) {
        const char *str = ::getenv( "TERM" );
        if ( !str){
            std::cerr << "Warning: TERM environment variable not set." << std::endl;
        } else {
            switch ( tgetent( buf, str)) {
            case 0:
                std::cerr << "Warning: No terminal entry '" << str << "'. " << std::endl;
                break;
            case -1:
                std::cerr << "Warning: terminfo database culd not be found." << std::endl;
                break;
            case 1: // success
                ho = tgetstr( const_cast<char *>("ho"), &table );   // home
                cl = tgetstr( const_cast<char *>("cl"), &table );   // clear screen
                ce = tgetstr( const_cast<char *>("ce"), &table );   // clear to end of line
                cd = tgetstr( const_cast<char *>("cd"), &table );   // clear to end of screen
                break;
            }
        }
    }
#endif
    if ( !cfg.opt_legacy) {
        os << cl;                    // clear screen
    }
    while( phase == BE_PHASE_1 ){
        // get screen size change if we can!
        cols = terminal_width( cols);
        time_t rawtime = time ( 0 );
        struct tm timeinfo = *( localtime( &rawtime ));
        std::map<std::string,std::string> stats = ss.get_realtime_stats();

        stats["elapsed_time"] = master_timer.elapsed_text();
        if ( fraction_done ) {
            double done = *(fraction_done);
            stats[FRACTION_READ] = std::to_string( done * 100) + std::string( " %" );
            stats[ESTIMATED_TIME_REMAINING] = master_timer.eta_text( done );
            stats[ESTIMATED_DATE_COMPLETION] = master_timer.eta_date( done );

            // print the legacy status
            if ( cfg.opt_legacy) {
                char buf1[64], buf2[64];
                snprintf( buf1, sizeof( buf1), "%2d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
                snprintf( buf2, sizeof( buf2), "(%.2f%%)", done * 100);
                uint64_t max_offset = strtoll( stats[ scanner_set::MAX_OFFSET ].c_str() , nullptr, 10);

                os  << buf1 << " Offset " << max_offset / ( 1000*1000) << "MB "
                          << buf2 << " Done in " << stats[ESTIMATED_TIME_REMAINING]
                          << " at " << stats[ESTIMATED_DATE_COMPLETION] << std::endl;
            }
        }
        if ( !cfg.opt_legacy) {
            os << ho << "bulk_extractor      " << asctime( &timeinfo) << "  " << std::endl;
            for( const auto &it : stats ){
                os << it.first << ": " << it.second;
                if ( ce[0] ){
                    os << ce;
                } else {
                    // Space out to the 50 column to erase any junk
                    int spaces = 50 - ( it.first.size() + it.second.size());
                    for( int i=0;i<spaces;i++){
                        os << " ";
                    }
                }
                os << std::endl;
            }
            if ( fraction_done ){
                if ( cols>10){
                    double done = *fraction_done;
                    int before = ( cols - 3) * done;
                    int after  = ( cols - 3) * ( 1.0 - done );
                    os << std::string( before,'=') << '>' << std::string( after,'.') << '|' << ce << std::endl;
                }
            }
            os << cd << std::endl << std::endl;
        }
        std::this_thread::sleep_for( std::chrono::seconds( cfg.opt_notify_rate ));
    }
    return nullptr;
}

void notify_thread::start_notify_thread( )
{
    the_notify_thread = new std::thread( &notify_thread::run, this);    // launch the notify thread
}

void notify_thread::join()
{
    assert( phase!= BE_PHASE_1 );
    if (the_notify_thread != nullptr) {
        the_notify_thread->join();
        delete the_notify_thread;
        the_notify_thread = nullptr;
    }
}
