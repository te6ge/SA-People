// Paper cashbook, drawn with the shared primitives. No browser, textures or extra assets.
inline bool ledgerPage(Page page) {
    return page==Page::Grove || page==Page::GroveDonate || page==Page::GroveUpgrades ||
        page==Page::GroveSupplies || page==Page::GroveConfirm;
}
inline Page ledgerBack(Page page,unsigned mode) {
    return page==Page::GroveConfirm?(mode?Page::GroveUpgrades:Page::GroveDonate):Page::Grove;
}
// A ledger session owns its page/cursor separately from the last phone screen.
struct LedgerSession {
    State saved; bool active=false;
    void open(State &screen) {
        if(active)return;
        saved=screen;if(ledgerPage(saved.page)){saved.page=Page::Contacts;saved.cursor=0;}
        screen={};screen.page=Page::Grove;active=true;
    }
    void close(State &screen) {
        if(active){screen=std::move(saved);saved={};active=false;}
        if(ledgerPage(screen.page)){screen.page=Page::Contacts;screen.cursor=0;screen.notice.clear();}
    }
};
inline panel::Layout ledgerLayout(float width,float height,float preference=1) {
    const float scale=std::min({(width-48)/940.0f,(height-64)/580.0f,
        .95f*std::clamp(preference,0.75f,1.25f)});
    return {(width-940*scale)/2,(height-580*scale)/2,scale};
}
inline panel::Layout viewLayout(Page page,float width,float height,float preference=1) {
    return ledgerPage(page)?ledgerLayout(width,height,preference):layout(width,height,preference);
}
inline panel::Scene composeLedger(const people::Registry &r,const State &s,const std::string &clock) {
    panel::Scene out;using panel::Align;using panel::Color;
    const Color ink{54,45,33,255},muted{113,92,65,255},red{133,57,43,255};
    auto box=[&](float x,float y,float w,float h,Color c){out.rectangles.push_back({x,y,w,h,c});};
    auto text=[&](float x,float y,float w,float size,const std::string &value,Color c=Color{54,45,33,255}) {
        out.texts.push_back({x,y,w,size,locale::tr(value),c,Align::Left,false});
    };
    auto wrapped=[&](float x,float y,float w,const std::string &value,unsigned cells,unsigned maxLines,float size=14.0f) {
        const auto lines=locale::wrap(locale::tr(value),cells);
        for(unsigned n=0;n<lines.size() && n<maxLines;++n)text(x,y+n*(size+4),w,size,lines[n]);
    };
    box(10,12,930,568,{0,0,0,110});box(0,0,930,568,{58,43,29,255});
    box(8,7,914,551,{110,79,44,255});box(16,14,444,534,{226,213,180,255});
    box(472,14,442,534,{236,224,192,255});box(457,14,18,534,{94,72,47,255});
    box(452,14,5,534,{171,146,105,255});box(475,14,4,534,{185,160,116,255});
    for(unsigned n=0;n<16;++n) {
        const float y=124+n*25.0f;
        box(40,y,392,1,{193,180,149,255});box(500,y,388,1,{202,187,153,255});
    }
    box(58,115,1,400,{166,99,78,170});box(517,115,1,400,{166,99,78,170});
    box(889,25,14,59,{85,102,69,255});
    text(45,39,370,30,"GROVE_LEDGER_TITLE");text(47,80,365,13,"GROVE_LEDGER_SUBTITLE",muted);
    text(505,45,365,23,s.page==Page::GroveDonate?"GROVE_DONATE":s.page==Page::GroveUpgrades?"GROVE_UPGRADES":s.page==Page::GroveSupplies?"GROVE_SUPPLIES":s.page==Page::GroveConfirm?"GROVE_CONFIRM_TITLE":"GROVE_TITLE");
    text(506,82,355,12,clock,muted);
    box(872,28,38,36,{130,63,45,255});text(883,34,26,23,"X",{245,232,203,255});
    text(75,143,350,24,locale::format("GROVE_BALANCE",std::to_string(r.grove.balance)));
    text(75,192,350,16,locale::format("GROVE_TOTAL",std::to_string(r.grove.contributed)));
    unsigned invested=0;for(unsigned p=0;p<grove::Count;++p)for(unsigned tier=0;tier<r.grove.level[p];++tier)invested+=people::GroveFund::cost(p,tier);
    text(75,235,350,16,locale::format("GROVE_INVESTED",std::to_string(invested)));
    wrapped(75,293,345,s.groveHere?"GROVE_LEDGER_HERE":"GROVE_LEDGER_VISIT",36,3);
    wrapped(75,383,345,"GROVE_LEDGER_NOTE",36,4,12);
    if(!s.notice.empty())wrapped(535,145,345,s.notice,34,11,18);
    else if(s.page==Page::GroveConfirm) {
        auto message=locale::format(s.groveMode?"GROVE_CONFIRM_UPGRADE":"GROVE_CONFIRM_DONATE",std::to_string(s.groveAmount));
        if(s.groveMode)message+=" "+locale::tr(grove::name(s.groveProject));
        wrapped(535,145,342,message,34,7,18);
        const char *choices[]={"REL_CONFIRM_YES","CONFIRM_CANCEL"};
        for(unsigned row=0;row<2;++row) {
            const float y=365+row*46.0f;
            if(s.cursor==row)box(532,y+24,342,2,red);
            text(540,y,332,20,choices[row],s.cursor==row?red:ink);
        }
    } else if(s.page==Page::Grove || s.page==Page::GroveDonate) {
        const char *choices[]={"GROVE_DONATE","GROVE_UPGRADES","GROVE_SUPPLIES","GROVE_ROUTE"};
        const unsigned amounts[]={100,1000,10000};
        const unsigned count=s.page==Page::Grove?4u:3u;
        for(unsigned row=0;row<count;++row) {
            const float y=151+row*65.0f;
            if(s.cursor==row)box(535,y+27,345,2,red);
            text(542,y,332,20,s.page==Page::Grove?locale::tr(choices[row]):locale::format("GROVE_AMOUNT",std::to_string(amounts[row])),s.cursor==row?red:ink);
        }
    } else {
        for(unsigned row=0;row<grove::Count;++row) {
            const float y=137+row*76.0f;
            if(s.cursor==row)box(534,y+26,348,2,red);
            auto label=locale::tr(grove::name(row))+"  "+std::to_string(r.grove.level[row])+"/2";
            const auto price=people::GroveFund::cost(row,r.grove.level[row]);
            if(s.page==Page::GroveUpgrades)label+=price?"  $"+std::to_string(price):"  MAX";
            text(540,y,338,17,label,s.cursor==row?red:ink);
            wrapped(541,y+32,333,grove::description(row),40,2,12);
        }
    }
    text(43,521,390,12,"CJ JOHNSON  /  GROVE STREET",muted);
    text(503,521,375,11,"GROVE_KEYS",muted);
    // Draw the software cursor inside the book; GTA can keep its normal mouse capture.
    box(s.ledgerX,s.ledgerY,2,18,{250,245,221,255});
    for(unsigned line=0;line<12;++line)box(s.ledgerX+line,s.ledgerY+line,2,2,{52,38,25,255});
    box(s.ledgerX,s.ledgerY,2,18,{52,38,25,255});
    return out;
}
